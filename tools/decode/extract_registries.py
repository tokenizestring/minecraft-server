import json, os, sys

ROOT = os.path.dirname(os.path.abspath(__file__))

SYNCED = {
    '26_2': ['worldgen/biome', 'chat_type', 'trim_pattern', 'trim_material', 'wolf_variant', 'wolf_sound_variant', 'pig_variant', 'pig_sound_variant', 'frog_variant', 'cat_variant', 'cat_sound_variant', 'cow_sound_variant', 'cow_variant', 'chicken_sound_variant', 'chicken_variant', 'zombie_nautilus_variant', 'painting_variant', 'sulfur_cube_archetype', 'dimension_type', 'damage_type', 'banner_pattern', 'enchantment', 'jukebox_song', 'instrument', 'test_environment', 'test_instance', 'dialog', 'world_clock', 'timeline'],
    '26_3': ['worldgen/biome', 'chat_type', 'trim_pattern', 'trim_material', 'wolf_variant', 'wolf_sound_variant', 'pig_variant', 'pig_sound_variant', 'frog_variant', 'cat_variant', 'cat_sound_variant', 'cow_sound_variant', 'cow_variant', 'chicken_sound_variant', 'chicken_variant', 'zombie_nautilus_variant', 'painting_variant', 'sulfur_cube_archetype', 'dimension_type', 'damage_type', 'banner_pattern', 'enchantment', 'jukebox_song', 'instrument', 'test_environment', 'test_instance', 'dialog', 'world_clock', 'timeline', 'decorated_pot_pattern', 'block_transformer', 'worldgen/block_state_provider'],
}

def list_entries(folder):
    names = []
    for base, _, files in os.walk(folder):
        for f in files:
            if f.endswith('.json'):
                rel = os.path.relpath(os.path.join(base, f), folder).replace('\\', '/')[:-5]
                names.append('minecraft:' + rel)
    return sorted(names)

def build(tag):
    gen = os.path.join(ROOT, '..', 'reference', 'gen_' + tag)
    data = os.path.join(gen, 'data', 'minecraft')
    static = json.load(open(os.path.join(gen, 'reports', 'registries.json')))

    ids = {}
    registries = []
    for reg in SYNCED[tag]:
        entries = list_entries(os.path.join(data, reg))
        assert entries, 'empty registry ' + reg
        registries.append(('minecraft:' + reg, entries))
        ids['minecraft:' + reg] = {name: index for index, name in enumerate(entries)}
    for reg, body in static.items():
        if reg not in ids:
            ids[reg] = {name: entry['protocol_id'] for name, entry in body['entries'].items()}

    tag_root = os.path.join(data, 'tags')
    raw = {}
    for base, _, files in os.walk(tag_root):
        for f in files:
            if f.endswith('.json'):
                rel = os.path.relpath(os.path.join(base, f), tag_root).replace('\\', '/')[:-5]
                parts = rel.split('/')
                for cut in range(len(parts) - 1, 0, -1):
                    reg = 'minecraft:' + '/'.join(parts[:cut])
                    if reg in ids:
                        raw.setdefault(reg, {})['minecraft:' + '/'.join(parts[cut:])] = json.load(open(os.path.join(base, f)))['values']
                        break

    def resolve(reg, name, seen):
        out = []
        for value in raw[reg][name]:
            required = True
            if isinstance(value, dict):
                required = value.get('required', True)
                value = value['id']
            if value.startswith('#'):
                inner = value[1:]
                if inner in raw[reg] and inner not in seen:
                    out += resolve(reg, inner, seen | {inner})
            elif value in ids[reg]:
                out.append(ids[reg][value])
            elif required:
                raise SystemExit('unresolved %s in %s %s' % (value, reg, name))
        return list(dict.fromkeys(out))

    tags = []
    for reg in sorted(raw):
        tags.append((reg, [(name, resolve(reg, name, {name})) for name in sorted(raw[reg])]))
    return registries, tags, ids

def write(tag, registries, tags):
    path = os.path.join(ROOT, '..', '..', 'data', 'java', tag + '.txt')
    with open(path, 'w', newline='\n') as f:
        for reg, entries in registries:
            f.write('registry %s\n' % reg)
            for e in entries:
                f.write(e + '\n')
        for reg, items in tags:
            f.write('tags %s\n' % reg)
            for name, values in items:
                f.write(name + ''.join(' %d' % v for v in values) + '\n')
    print('wrote', os.path.normpath(path), os.path.getsize(path), 'bytes', len(registries), 'registries', sum(len(t[1]) for t in tags), 'tags')

def rvarint(b, p):
    r = s = 0
    while True:
        x = b[p]; p += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            return r, p

def rstr(b, p):
    n, p = rvarint(b, p)
    return b[p:p + n].decode(), p + n

def verify(tag, registries, tags, ids):
    blob_path = os.path.join(ROOT, '..', 'captures', 'capture_' + tag, 'registry.bin')
    if not os.path.exists(blob_path):
        print('no vanilla reference for', tag)
        return
    blob = open(blob_path, 'rb').read()
    vanilla_regs = {}
    vanilla_tags = {}
    p = 0
    while p < len(blob):
        n, p = rvarint(blob, p)
        body = blob[p:p + n]; p += n
        pid, q = rvarint(body, 0)
        if pid == 7:
            reg, q = rstr(body, q)
            count, q = rvarint(body, q)
            names = []
            for _ in range(count):
                name, q = rstr(body, q)
                assert body[q] == 0, 'vanilla sent inline data for ' + reg
                q += 1
                names.append(name)
            vanilla_regs[reg] = names
        elif len(body) > 1000:
            count, q = rvarint(body, q)
            for _ in range(count):
                reg, q = rstr(body, q)
                tcount, q = rvarint(body, q)
                for _ in range(tcount):
                    name, q = rstr(body, q)
                    vcount, q = rvarint(body, q)
                    values = []
                    for _ in range(vcount):
                        v, q = rvarint(body, q); values.append(v)
                    vanilla_tags.setdefault(reg, {})[name] = values
    problems = 0
    ours = dict(registries)
    if list(ours) != list(vanilla_regs):
        problems += 1; print('registry order/list differs', set(ours) ^ set(vanilla_regs))
    for reg, names in vanilla_regs.items():
        if set(names) != set(ours.get(reg, [])):
            problems += 1; print('entries differ', reg, set(names) ^ set(ours.get(reg, [])))
    def to_names(reg, values, order):
        if reg in order:
            return {order[reg][v] for v in values}
        rev = {i: n for n, i in ids[reg].items()}
        return {rev[v] for v in values}
    our_tags = {reg: dict(items) for reg, items in tags}
    our_order = {reg: entries for reg, entries in registries}
    if set(our_tags) != set(vanilla_tags):
        problems += 1; print('tag registries differ: only ours', set(our_tags) - set(vanilla_tags), 'only vanilla', set(vanilla_tags) - set(our_tags))
    for reg in set(our_tags) & set(vanilla_tags):
        if set(our_tags[reg]) != set(vanilla_tags[reg]):
            problems += 1; print('tag names differ', reg, list(set(our_tags[reg]) ^ set(vanilla_tags[reg]))[:5])
        for name in set(our_tags[reg]) & set(vanilla_tags[reg]):
            a = to_names(reg, our_tags[reg][name], our_order)
            b = to_names(reg, vanilla_tags[reg][name], vanilla_regs)
            if a != b:
                problems += 1; print('tag contents differ', reg, name, list(a ^ b)[:5])
    print('verify', tag, 'problems', problems)

for tag in sys.argv[1:] or ['26_2', '26_3']:
    registries, tags, ids = build(tag)
    verify(tag, registries, tags, ids)
    write(tag, registries, tags)
