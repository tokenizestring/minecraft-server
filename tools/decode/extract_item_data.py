import json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
REF = os.path.join(HERE, '..', 'reference', 'gen_26_3')
OUT = os.path.join(HERE, '..', '..', 'data', 'java')

CHANCE = ('minecraft:random_chance', 'minecraft:table_bonus', 'minecraft:random_chance_with_enchanted_bonus')
TOOL = ('minecraft:tool/can_silk_touch', 'minecraft:tool/can_shear')

def conditions(entry):
    found = entry.get('conditions', []) + ([entry['condition']] if 'condition' in entry else [])
    out = []
    while found:
        c = found.pop()
        if isinstance(c, str):
            out.append(c)
        else:
            out.append(c.get('type'))
            found += c.get('terms', [])
    return out

def usable(entry):
    kinds = conditions(entry)
    return entry.get('type') == 'minecraft:item' and not any(k in CHANCE or k in TOOL or k == 'minecraft:any_of' and 'minecraft:tool/can_shear' in kinds for k in kinds)

def drop(table):
    for pool in table.get('pools', [])[:1]:
        for entry in pool.get('entries', [])[:1]:
            children = entry.get('children', [entry]) if entry.get('type') == 'minecraft:alternatives' else [entry]
            picks = [c for c in children if usable(c)]
            if picks:
                return picks[-1]['name']
    return '-'

stacks = []
for name in sorted(os.listdir(os.path.join(REF, 'reports', 'minecraft', 'components', 'item'))):
    size = json.load(open(os.path.join(REF, 'reports', 'minecraft', 'components', 'item', name)))['components'].get('minecraft:max_stack_size', 64)
    if size != 64:
        stacks.append('minecraft:%s %d' % (name[:-5], size))

drops = []
blocks = json.load(open(os.path.join(REF, 'reports', 'blocks.json')))
loot = os.path.join(REF, 'data', 'minecraft', 'loot_table', 'blocks')
for name in sorted(blocks):
    path = os.path.join(loot, name.split(':')[1] + '.json')
    drops.append('%s %s' % (name, drop(json.load(open(path))) if os.path.exists(path) else '-'))

open(os.path.join(OUT, 'stacks.txt'), 'w', newline='\n').write('\n'.join(stacks) + '\n')
open(os.path.join(OUT, 'drops.txt'), 'w', newline='\n').write('\n'.join(drops) + '\n')
print(len(stacks), 'stack sizes,', len(drops), 'block drops')
