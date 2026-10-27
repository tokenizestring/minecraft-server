import json, os, sys, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
JAR = sys.argv[1] if len(sys.argv) > 1 else os.path.expandvars(r'%APPDATA%\.minecraft\versions\26.3\26.3.jar')
REPORTS = {version: os.path.join(HERE, '..', 'reference', 'gen_' + version, 'reports', 'blocks.json') for version in ('26_2', '26_3')}
ITEMS = os.path.join(HERE, '..', '..', 'data', 'java', 'items_26_3.txt')
OUT = os.path.join(HERE, '..', '..', 'data', 'java', 'crops.txt')

RANDOM_TICK_SPEED = 3
SECTION_VOLUME = 4096
CROP_FIELD_SPEED = 10.0

def crop_chance():
    return 1.0 / (int(25.0 / CROP_FIELD_SPEED) + 1)

CROPS = {
    'wheat': {'soil': 'farmland', 'chance': crop_chance(), 'legacy': (59, 7)},
    'carrots': {'soil': 'farmland', 'chance': crop_chance(), 'legacy': (141, 7)},
    'potatoes': {'soil': 'farmland', 'chance': crop_chance(), 'legacy': (142, 7)},
    'beetroots': {'soil': 'farmland', 'chance': crop_chance() * 2.0 / 3.0, 'legacy': (59, 7)},
    'nether_wart': {'soil': 'soul_sand', 'chance': 0.1, 'legacy': (115, 3)},
}

jar = zipfile.ZipFile(JAR)
reports = {version: json.load(open(path)) for version, path in REPORTS.items()}

def loot(name):
    return json.loads(jar.read('data/minecraft/loot_table/blocks/%s.json' % name))['pools']

def ages(version, block):
    states = sorted(reports[version][block]['states'], key=lambda state: int(state['properties']['age']))
    first = states[0]['id']
    assert all(state['id'] == first + int(state['properties']['age']) for state in states), block
    return first, len(states) - 1

def mature_drops(name):
    pools = loot(name)
    head = pools[0]['entries'][0]
    immature = head['children'][1]['name'] if head['type'] == 'minecraft:alternatives' else head['name']
    produce = head['children'][0]['name'] if head['type'] == 'minecraft:alternatives' else head['name']
    counts = {produce: [1, 1, 0]}
    probability = 0.0
    modifiers = head.get('modifier', []) if isinstance(head.get('modifier', []), list) else [head['modifier']]
    for modifier in modifiers:
        if modifier['type'] == 'minecraft:set_count':
            counts[produce] = [modifier['count']['min'], modifier['count']['max'], 0]
    for pool in pools[1:]:
        entry = pool['entries'][0]
        bonus = entry.get('modifier', {})
        if 'condition' in entry or bonus.get('formula') != 'minecraft:binomial_with_bonus_count':
            continue
        low, high, trials = counts.get(entry['name'], [0, 0, 0])
        counts[entry['name']] = [low + 1, high + 1, trials + bonus['parameters']['extra']]
        probability = bonus['parameters']['probability']
    extra = [item for item in counts if item != produce]
    return immature, produce, counts[produce], (extra[0], counts[extra[0]]) if extra else ('-', [0, 0, 0]), probability

rows = ['# crop block max_age stage_ticks state_26_2 state_26_3 legacy_block legacy_max soil seed produce min max trials extra min max trials probability']
interval = SECTION_VOLUME / RANDOM_TICK_SPEED

for short, info in CROPS.items():
    block = 'minecraft:' + short
    first_26_2, top_26_2 = ages('26_2', block)
    first_26_3, top = ages('26_3', block)
    assert top == top_26_2, block
    seed, produce, counts, (extra, extra_counts), probability = mature_drops(short)
    ticks = int(round(interval / info['chance']))
    legacy, legacy_max = info['legacy']
    rows.append('crop %s %d %d %d %d %d %d minecraft:%s %s %s %d %d %d %s %d %d %d %.7f' % (block, top, ticks, first_26_2, first_26_3, legacy, legacy_max, info['soil'], seed, produce, counts[0], counts[1], counts[2], extra, extra_counts[0], extra_counts[1], extra_counts[2], probability))

rows.append('# till block result')
tags = {}

def tag(name):
    if name not in tags:
        values = json.loads(jar.read('data/minecraft/tags/block/%s.json' % name.split(':', 1)[1]))['values']
        tags[name] = [value for entry in values for value in (tag(entry[1:]) if entry.startswith('#') else [entry])]
    return tags[name]

for transform in json.loads(jar.read('data/minecraft/block_transformer/hoe.json')):
    for rule in transform['block_state_provider']['rules']:
        test = rule['if_true']
        predicates = test['predicates'] if test['type'] == 'minecraft:all_of' else [test]
        source = next(predicate for predicate in predicates if 'offset' not in predicate)
        found = tag(source['tag']) if source['type'] == 'minecraft:matching_block_tag' else [source['blocks']]
        rows.extend('till %s %s' % (block, rule['then']['id']) for block in found)

rows.append('# chance block item chance')

for name in sorted(reports['26_3']):
    path = 'data/minecraft/loot_table/blocks/%s.json' % name.split(':', 1)[1]
    pools = json.loads(jar.read(path)).get('pools', []) if path in jar.namelist() else []
    for pool in pools:
        for entry in pool['entries']:
            for child in entry.get('children', [entry]):
                condition = child.get('condition', {})
                if isinstance(condition, dict) and condition.get('type') == 'minecraft:random_chance' and child.get('name') == 'minecraft:wheat_seeds':
                    rows.append('chance %s %s %.4f' % (name, child['name'], condition['chance']))

rows.append('# hoe item')
rows.extend('hoe ' + line.split()[0] for line in open(ITEMS) if line.split() and line.split()[0].endswith('_hoe'))

open(OUT, 'w', newline='\n').write('\n'.join(rows) + '\n')
print(len(rows), 'lines,', len(CROPS), 'crops')
