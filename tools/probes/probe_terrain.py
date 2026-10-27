import time, sys, os, json, struct, random, gzip, array
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode', 'convert')]
from clients import *
import schem_to_world as conv

SHIFT = (384, 16, 368)
SCHEM = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'data', 'spawn', 'spawn.schem')
REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'reference', 'gen_%s', 'reports', 'blocks.json')
VIEW = 6

def state_names(tag):
    names = {}
    for name, block in json.load(open(REPORT % tag)).items():
        for st in block['states']:
            props = st.get('properties', {})
            names[st['id']] = name + ('[' + ','.join('%s=%s' % kv for kv in sorted(props.items())) + ']' if props else '')
    return names

def load_schematic():
    sch = conv.Nbt(gzip.decompress(open(SCHEM, 'rb').read())).root()['Schematic']
    palette = {i: conv.canonical(s)[2] for s, i in sch['Blocks']['Palette'].items()}
    values = array.array('H'); v = 0; sh = 0
    for b in sch['Blocks']['Data']:
        if b & 0x80: v |= (b & 0x7F) << sh; sh += 7
        else: values.append(v | (b << sh)); v = 0; sh = 0
    return sch['Width'], sch['Height'], sch['Length'], palette, values

def expected_block(world, wx, wy, wz):
    W, H, L, palette, values = world
    lx, ly, lz = wx + SHIFT[0], wy + SHIFT[1], wz + SHIFT[2]
    if 0 <= lx < W and 0 <= ly < H and 0 <= lz < L:
        return palette[values[(ly * L + lz) * W + lx]]
    return 'minecraft:air'

def decode_chunk(body):
    x, z = struct.unpack('>ii', body[:8]); p = 8
    count, p = rvarint(body, p)
    for _ in range(count):
        kind, p = rvarint(body, p); n, p = rvarint(body, p); p += 8 * n
    size, p = rvarint(body, p); end = p + size
    sections = []
    for _ in range(24):
        non_air, fluid = struct.unpack('>HH', body[p:p + 4]); p += 4
        bits = body[p]; p += 1
        if bits == 0:
            value, p = rvarint(body, p); blocks = [value] * 4096
        else:
            palette = None
            if bits <= 8:
                n, p = rvarint(body, p); palette = []
                for _ in range(n):
                    value, p = rvarint(body, p); palette.append(value)
            per = 64 // bits; longs = (4096 + per - 1) // per
            words = struct.unpack('>%dQ' % longs, body[p:p + 8 * longs]); p += 8 * longs
            mask = (1 << bits) - 1
            blocks = [(words[i // per] >> ((i % per) * bits)) & mask for i in range(4096)]
            if palette is not None:
                blocks = [palette[b] for b in blocks]
        biome_bits = body[p]; p += 1
        assert biome_bits == 0
        _, p = rvarint(body, p)
        sections.append((non_air, fluid, blocks))
    assert p == end, ('section bytes', p, end)
    entities, p = rvarint(body, p)
    masks = []
    for _ in range(4):
        n, p = rvarint(body, p)
        if BYTE_BITSETS: masks.append(int.from_bytes(body[p:p + n], 'little')); p += n
        else: masks.append(sum(struct.unpack('>Q', body[p + 8 * i:p + 8 * i + 8])[0] << (64 * i) for i in range(n))); p += 8 * n
    count, p = rvarint(body, p); sky = {}
    bits = [i for i in range(26) if masks[0] >> i & 1]
    assert len(bits) == count, ('sky arrays', len(bits), count)
    for bit in bits:
        n, p = rvarint(body, p); assert n == 2048; sky[bit - 1] = body[p:p + n]; p += n
    blocks_light, p = rvarint(body, p)
    assert blocks_light == bin(masks[1]).count('1'), ('block arrays', blocks_light, masks[1])
    for _ in range(blocks_light):
        n, p = rvarint(body, p); assert n == 2048; p += n
    assert p == len(body), 'trailing light bytes'
    LIGHT[(x, z)] = sky
    return x, z, sections

LIGHT = {}
BYTE_BITSETS = True
def sky_at(wx, wy, wz):
    sky = LIGHT[(wx >> 4, wz >> 4)]; section = (wy + 64) >> 4
    if section not in sky: return 15
    i = ((wy & 15) * 16 + (wz & 15)) * 16 + (wx & 15)
    return (sky[section][i // 2] >> ((i & 1) * 4)) & 15

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

print('loading schematic...')
world = load_schematic()
for protocol, tag in ((777, '26_3'), (776, '26_2')):
    names = state_names(tag)
    BYTE_BITSETS = tag == '26_3'
    cb, sb = packet_ids(tag)
    client = Java('Terrain' + tag, protocol); client.join(); time.sleep(3.0)
    with client.lock:
        chunks = [body for pid, body in client.log if pid == cb['level_chunk_with_light']]
    check('%s receives %d chunks' % (tag, (2 * VIEW + 1) ** 2), len(chunks) == (2 * VIEW + 1) ** 2, len(chunks))
    random.seed(1); wrong = []; checked = 0; air_mismatch = 0
    for body in chunks:
        cx, cz, sections = decode_chunk(body)
        for index, (non_air, fluid, blocks) in enumerate(sections):
            sy = index - 4
            solid = sum(1 for b in blocks if not names[b].startswith(('minecraft:air', 'minecraft:cave_air', 'minecraft:void_air')))
            if solid != non_air: air_mismatch += 1
            for _ in range(48):
                i = random.randrange(4096)
                wx, wy, wz = cx * 16 + (i & 15), sy * 16 + (i >> 8), cz * 16 + ((i >> 4) & 15)
                want = expected_block(world, wx, wy, wz)
                got = names[blocks[i]]
                checked += 1
                if got != want and len(wrong) < 5: wrong.append(((wx, wy, wz), want, got))
                elif got != want: wrong.append(None)
    check('%s %d sampled blocks match the schematic' % (tag, checked), not wrong, wrong[:5])
    check('%s non-air counts correct in every section' % tag, air_mismatch == 0, air_mismatch)
    check('%s sky light 15 above spawn, 0 inside the stone below' % tag, sky_at(12, 61, 7) == 15 and sky_at(12, 50, 7) == 0, (sky_at(12, 61, 7), sky_at(12, 50, 7)))
    with client.lock:
        client.log.clear()
    for step in range(1, 4):
        client.move(12.5 + step * 16, 60.0, 7.5, 0.0); time.sleep(0.4)
    time.sleep(1.0)
    with client.lock:
        log = list(client.log)
    new_chunks = [decode_chunk(b)[:2] for pid, b in log if pid == cb['level_chunk_with_light']]
    forgets = [struct.unpack('>ii', b)[::-1] for pid, b in log if pid == cb['forget_level_chunk']]
    centers = [b for pid, b in log if pid == cb['set_chunk_cache_center']]
    want_new = {(x, z) for x in range(VIEW + 1, VIEW + 4) for z in range(-VIEW, VIEW + 1)}
    want_forget = {(x, z) for x in range(-VIEW, -VIEW + 3) for z in range(-VIEW, VIEW + 1)}
    check('%s walking east 3 chunks streams the new strip' % tag, set(new_chunks) == want_new and len(new_chunks) == len(want_new), (len(new_chunks), len(want_new)))
    check('%s and unloads the old strip' % tag, set(forgets) == want_forget and len(forgets) == len(want_forget), (len(forgets), sorted(set(forgets) ^ want_forget)[:4]))
    check('%s center moved 3 times' % tag, len(centers) == 3, len(centers))
    client.close()
print('failures:', fails)
