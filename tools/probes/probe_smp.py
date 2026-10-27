import time, sys, os, struct, json
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'reference', 'gen_26_3', 'reports', 'blocks.json')
VIEW = 6

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

names = {}
for name, block in json.load(open(REPORT)).items():
    for state in block['states']:
        names[state['id']] = name

def sections_of(body):
    x, z = struct.unpack('>ii', body[:8]); p = 8
    count, p = rvarint(body, p)
    for _ in range(count):
        kind, p = rvarint(body, p); n, p = rvarint(body, p); p += 8 * n
    size, p = rvarint(body, p); end = p + size; out = []
    for _ in range(24):
        non_air, fluid = struct.unpack('>HH', body[p:p + 4]); p += 4
        bits = body[p]; p += 1
        if bits == 0:
            value, p = rvarint(body, p); blocks = [value] * 4096
        else:
            n, p = rvarint(body, p); palette = []
            for _ in range(n):
                value, p = rvarint(body, p); palette.append(value)
            per = 64 // bits; longs = (4096 + per - 1) // per
            words = struct.unpack('>%dQ' % longs, body[p:p + 8 * longs]); p += 8 * longs
            blocks = [palette[(words[i // per] >> ((i % per) * bits)) & ((1 << bits) - 1)] for i in range(4096)]
        p += 1; _, p = rvarint(body, p)
        solid = sum(1 for b in blocks if names[b] != 'minecraft:air')
        assert solid == non_air, ('non air count', solid, non_air)
        out.append(blocks)
    assert p == end
    return x, z, out

cb, sb = packet_ids('26_3')
c = Java('SmpProbe', 777); c.join(); time.sleep(1.0)
with c.lock: c.log.clear()
started = time.time()
c.send(sb['chat_command'], mstr('smp'))
deadline = time.time() + 15
while time.time() < deadline:
    with c.lock: got = sum(1 for p, b in c.log if p == cb['level_chunk_with_light'])
    if got >= (2 * VIEW + 1) ** 2: break
    time.sleep(0.02)
elapsed = time.time() - started
with c.lock: log = list(c.log)
chunks = [b for p, b in log if p == cb['level_chunk_with_light']]
forgets = [b for p, b in log if p == cb['forget_level_chunk']]
positions = [b for p, b in log if p == cb['player_position']]
notes = [b for p, b in log if p == cb['system_chat']]
check('switching to the smp unloads the spawn chunks', len(forgets) == (2 * VIEW + 1) ** 2, len(forgets))
check('and streams %d generated chunks' % ((2 * VIEW + 1) ** 2), len(chunks) == (2 * VIEW + 1) ** 2, len(chunks))
print('   169 generated chunks arrived in %.0f ms (first time, nothing cached)' % (elapsed * 1000))
check('welcome notice', any(b'welcome to smp' in n for n in notes), notes)
spawn = struct.unpack('>ddd', positions[-1][1:25]) if positions else None
check('teleported onto land above sea level', spawn and spawn[1] > 63, spawn)
counts = {}
decoded = {}
for body in chunks:
    x, z, sections = sections_of(body)
    decoded[(x, z)] = sections
    for blocks in sections:
        for b in set(blocks):
            counts[names[b]] = counts.get(names[b], 0) + 1
sx, sy, sz = int(spawn[0] // 1), int(spawn[1]), int(spawn[2] // 1)
below = decoded[(sx >> 4, sz >> 4)][(sy - 1 + 64) >> 4][((sy - 1) & 15) * 256 + (sz & 15) * 16 + (sx & 15)]
check('there is solid ground under the smp spawn', names[below] not in ('minecraft:air', 'minecraft:water'), names[below])
kinds = sorted(k.split(':')[1] for k in counts)
check('the world has real variety (stone, dirt, grass, ores, bedrock...)', all(k in kinds for k in ('stone', 'deepslate', 'dirt', 'grass_block', 'bedrock', 'coal_ore', 'iron_ore')), kinds)
c.move(spawn[0] + 64, spawn[1] + 30, spawn[2], 0.0); time.sleep(1.0)
with c.lock: more = sum(1 for p, b in c.log if p == cb['level_chunk_with_light']) - len(chunks)
check('walking 4 chunks streams new generated chunks', more == 4 * (2 * VIEW + 1), more)
with c.lock: c.log.clear()
c.send(sb['chat_command'], mstr('spawn')); time.sleep(1.0)
with c.lock: back = [b for p, b in c.log if p == cb['player_position']]
check('/spawn goes back to the spawn build', back and abs(struct.unpack('>d', back[-1][9:17])[0] - 60.0) < 0.01, back)
c.close()

c = Java('SmpProbe18', 47); c.join(); time.sleep(0.8)
with c.lock: c.log.clear()
c.send(0x01, mstr('/smp')); time.sleep(1.5)
with c.lock: chunks18 = [b for p, b in c.log if p == 0x21]
filled = [b for b in chunks18 if struct.unpack('>H', b[9:11])[0]]
check('1.8 gets generated smp chunks', len(filled) >= 150, (len(chunks18), len(filled)))
mask = struct.unpack('>H', filled[0][9:11])[0]; n, p = rvarint(filled[0], 11)
check('1.8 chunk sizes add up', n == bin(mask).count('1') * (8192 + 4096) + 256 and p + n == len(filled[0]), (n, len(filled[0])))
c.close()

r = Java('RollProbe', 777); r.join(); time.sleep(0.8)
r.send(sb['chat_command'], mstr('smp')); time.sleep(1.0)
with r.lock: r.log.clear()
r.send(sb['chat_command'], mstr('roll')); time.sleep(1.5)
with r.lock: log = list(r.log)
notes = [b for p, b in log if p == cb['system_chat']]
rolled = [b for p, b in log if p == cb['level_chunk_with_light']]
check('/roll regenerates the smp for players in it', any(b'smp rolled' in n for n in notes) and len(rolled) == (2 * VIEW + 1) ** 2, (notes, len(rolled)))
r.close()
print('failures:', fails)
