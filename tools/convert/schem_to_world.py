import gzip, struct, json, os, sys, array, hashlib, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
REPORTS = os.path.join(HERE, '..', 'reference', 'gen_%s', 'reports', 'blocks.json')
FLUID_BLOCKS = {'minecraft:water', 'minecraft:lava', 'minecraft:bubble_column', 'minecraft:kelp', 'minecraft:kelp_plant', 'minecraft:seagrass', 'minecraft:tall_seagrass'}
AIR_BLOCKS = {'minecraft:air', 'minecraft:cave_air', 'minecraft:void_air'}
CLEAR_WORDS = ('air', 'glass', 'pane', 'fence', 'wall', 'slab', 'stairs', 'torch', 'lantern', 'flower', 'tulip', 'orchid', 'allium', 'bluet', 'daisy', 'poppy', 'dandelion', 'cornflower', 'lily', 'rose_bush', 'peony', 'lilac', 'sunflower', 'short_grass', 'tall_grass', 'fern', 'sapling', 'vine', 'ladder', 'door', 'sign', 'banner', 'button', 'lever', 'rail', 'carpet', 'bars', 'chain', 'candle', 'head', 'skull', 'pot', 'rod', 'campfire', 'coral', 'scaffolding', 'sugar_cane', 'wheat', 'carrots', 'potatoes', 'beetroots', 'stem', 'dripleaf', 'spore_blossom', 'pressure_plate', 'bed', 'anvil', 'brewing_stand', 'cauldron', 'hopper', 'enchanting_table', 'cake', 'conduit', 'bell', 'grindstone', 'stonecutter', 'daylight_detector', 'repeater', 'comparator', 'redstone_wire', 'tripwire', 'chorus', 'cactus', 'pickle', 'frogspawn', 'roots', 'lichen', 'sculk_vein', 'pointed_dripstone', 'amethyst_cluster', 'amethyst_bud', 'chest', 'barrier', 'light', 'structure_void', 'beacon', 'honey_block', 'slime_block', 'piston_head', 'moving_piston', 'portal', 'fire', 'leaf_litter', 'petals', 'wildflowers', 'bush', 'egg', 'plant', 'sprouts', 'fungus', 'weeping', 'twisting', 'cave_vines', 'grate', 'spawner', 'vault', 'heavy_core', 'snow', 'mushroom', 'azalea', 'shelf', 'lectern', 'composter', 'farmland', 'dirt_path', 'bamboo', 'end_portal_frame', 'cobweb', 'kelp', 'seagrass', 'water', 'lava', 'ice', 'leaves')
SOLID_EXCEPTIONS = ('snow_block', 'mushroom_block', 'mushroom_stem', 'bamboo_block', 'bamboo_planks', 'bamboo_mosaic', 'stripped_bamboo_block', 'tinted_glass', 'copper_bulb', 'redstone_lamp', 'sea_lantern', 'glowstone', 'shroomlight', 'packed_ice', 'blue_ice', 'light_gray', 'light_blue', 'lightning')
DIM_WORDS = ('leaves', 'water', 'ice', 'cobweb', 'kelp', 'seagrass', 'bubble_column')

def light_class(name, props):
    short = name.split(':', 1)[1]
    if props.get('type') == 'double' or any(word in short for word in SOLID_EXCEPTIONS if word not in ('light_gray', 'light_blue', 'lightning')) and not any(w in short for w in ('_pane', 'stairs', 'slab', 'wall', 'fence', 'door', 'carpet', 'button', 'pressure_plate', 'sign', 'banner', 'bed', 'candle')):
        return 4
    if props.get('waterlogged') == 'true' or any(word in short for word in DIM_WORDS):
        return 8 if 'packed_ice' not in short and 'blue_ice' not in short else 4
    if any(word in short for word in CLEAR_WORDS) and not short.startswith(('light_gray_', 'light_blue_')):
        return 0
    if short.startswith(('light_gray_', 'light_blue_')) and not any(word in short.split('_', 2)[2] for word in CLEAR_WORDS):
        return 4
    if short.startswith(('light_gray_', 'light_blue_')):
        return 0
    return 4

class Nbt:
    def __init__(self, raw): self.raw, self.p = raw, 0
    def take(self, n): v = self.raw[self.p:self.p + n]; self.p += n; return v
    def u8(self): v = self.raw[self.p]; self.p += 1; return v
    def i32(self): return struct.unpack('>i', self.take(4))[0]
    def s(self): return self.take(struct.unpack('>H', self.take(2))[0]).decode('utf-8')
    def payload(self, t):
        if t == 1: return struct.unpack('>b', self.take(1))[0]
        if t == 2: return struct.unpack('>h', self.take(2))[0]
        if t == 3: return self.i32()
        if t == 4: return struct.unpack('>q', self.take(8))[0]
        if t == 5: return struct.unpack('>f', self.take(4))[0]
        if t == 6: return struct.unpack('>d', self.take(8))[0]
        if t == 7: n = self.i32(); return self.take(n)
        if t == 8: return self.s()
        if t == 9:
            inner = self.u8(); return [self.payload(inner) for _ in range(self.i32())]
        if t == 10:
            out = {}
            while True:
                inner = self.u8()
                if inner == 0: return out
                key = self.s(); out[key] = self.payload(inner)
        if t == 11: n = self.i32(); return list(struct.unpack('>%di' % n, self.take(4 * n)))
        if t == 12: n = self.i32(); return list(struct.unpack('>%dq' % n, self.take(8 * n)))
        raise ValueError('nbt tag %d' % t)
    def root(self):
        t = self.u8(); self.s(); return self.payload(t)

def canonical(state):
    name, _, rest = state.partition('[')
    props = dict(p.split('=', 1) for p in rest.rstrip(']').split(',')) if rest else {}
    key = name + ('[' + ','.join('%s=%s' % kv for kv in sorted(props.items())) + ']' if props else '')
    return name, props, key

def java_ids(tag):
    blocks = json.load(open(REPORTS % tag))
    ids = {}
    for name, block in blocks.items():
        for st in block['states']:
            props = st.get('properties', {})
            key = name + ('[' + ','.join('%s=%s' % kv for kv in sorted(props.items())) + ']' if props else '')
            ids[key] = st['id']
            if st.get('default'):
                ids.setdefault(name + '#default', st['id'])
    return ids

def pack(indices, bits):
    per = 64 // bits
    longs = []
    for start in range(0, 4096, per):
        value = 0
        for slot, index in enumerate(indices[start:start + per]):
            value |= index << (slot * bits)
        longs.append(value)
    return longs

def main(source, target, shift_x, shift_y, shift_z):
    started = time.time()
    schematic = Nbt(gzip.decompress(open(source, 'rb').read())).root()['Schematic']
    width, height, length = schematic['Width'], schematic['Height'], schematic['Length']
    offset = schematic['Offset']
    palette = {index: state for state, index in schematic['Blocks']['Palette'].items()}
    data = schematic['Blocks']['Data']
    print('schematic %dx%dx%d, %d states, data version %d' % (width, height, length, len(palette), schematic['DataVersion']))

    values = array.array('H'); value = 0; shift = 0
    for byte in data:
        if byte & 0x80:
            value |= (byte & 0x7F) << shift; shift += 7
        else:
            values.append(value | (byte << shift)); value = 0; shift = 0
    assert len(values) == width * height * length

    ids = {tag: java_ids(tag) for tag in ('26_2', '26_3')}
    entries = []
    for index in range(len(palette)):
        name, props, key = canonical(palette[index])
        row = [key]
        for tag in ('26_2', '26_3'):
            row.append(ids[tag].get(key, ids[tag].get(name + '#default', 0)))
        flags = (1 if name in AIR_BLOCKS else 0) | (2 if name in FLUID_BLOCKS or props.get('waterlogged') == 'true' else 0) | (0 if name in AIR_BLOCKS else light_class(name, props))
        row.append(flags)
        entries.append(row)
    air = [i for i, e in enumerate(entries) if e[3] & 1]

    spawn_local = (-offset[0], -offset[1], -offset[2])
    spawn = (spawn_local[0] - shift_x, spawn_local[1] - shift_y, spawn_local[2] - shift_z)
    chunks_x, chunks_z, sections_y = (width + 15) // 16, (length + 15) // 16, (height + 15) // 16
    min_chunk_x, min_chunk_z, min_section_y = -shift_x // 16, -shift_z // 16, -shift_y // 16

    layer = width * length
    unique = {}; sections = []; columns = []
    for cz in range(chunks_z):
        for cx in range(chunks_x):
            mask = 0; refs = []
            for sy in range(sections_y):
                block = array.array('H', [air[0]] * 4096)
                for y in range(16):
                    ly = sy * 16 + y
                    if ly >= height: break
                    for z in range(16):
                        lz = cz * 16 + z
                        if lz >= length: break
                        start = ly * layer + lz * width + cx * 16
                        row = values[start:start + min(16, width - cx * 16)]
                        block[y * 256 + z * 16:y * 256 + z * 16 + len(row)] = row
                states = sorted(set(block))
                if all(s in air for s in states):
                    continue
                digest = hashlib.blake2b(block.tobytes(), digest_size=16).digest()
                if digest not in unique:
                    local = {s: i for i, s in enumerate(states)}
                    bits = 0 if len(states) == 1 else max(4, (len(states) - 1).bit_length())
                    non_air = sum(1 for s in block if not entries[s][3] & 1)
                    fluid = sum(1 for s in block if entries[s][3] & 2)
                    longs = pack([local[s] for s in block], bits) if bits else []
                    unique[digest] = len(sections)
                    sections.append((states, non_air, fluid, bits, longs))
                mask |= 1 << sy
                refs.append(unique[digest])
            columns.append((mask, refs))

    out = bytearray(b'MCWB') + struct.pack('<I', 1)
    out += struct.pack('<3i', *spawn)
    out += struct.pack('<2i2I', min_chunk_x, min_chunk_z, chunks_x, chunks_z)
    out += struct.pack('<iI', min_section_y, sections_y)
    out += struct.pack('<I', len(entries))
    for key, id_26_2, id_26_3, flags in entries:
        encoded = key.encode()
        out += struct.pack('<H', len(encoded)) + encoded + struct.pack('<IIB', id_26_2, id_26_3, flags)
    out += struct.pack('<I', len(sections))
    for states, non_air, fluid, bits, longs in sections:
        out += struct.pack('<HHHB', len(states), non_air, fluid, bits)
        out += struct.pack('<%dH' % len(states), *states)
        out += struct.pack('<%dQ' % len(longs), *longs)
    for mask, refs in columns:
        out += struct.pack('<I', mask) + struct.pack('<%dI' % len(refs), *refs)
    open(target, 'wb').write(out)

    print('spawn %s, chunks x %d..%d z %d..%d, sections y %d..%d' % (spawn, min_chunk_x, min_chunk_x + chunks_x - 1, min_chunk_z, min_chunk_z + chunks_z - 1, min_section_y, min_section_y + sections_y - 1))
    print('%d sections stored (%d placed), %d palette states, %.1f MB, %.0f s' % (len(sections), sum(len(r) for m, r in columns), len(entries), len(out) / 1e6, time.time() - started))

if __name__ == '__main__':
    source = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'data', 'spawn', 'spawn.schem')
    target = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, 'data', 'spawn', 'world.bin')
    main(source, target, 384, 16, 368)
