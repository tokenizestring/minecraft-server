import time, sys, os, struct, json
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'data', 'java')
glow = {}
for line in open(os.path.join(DATA, 'light.txt')):
    state, level, _ = map(int, line.split())
    if level: glow[state] = level

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

cb3, sb3 = packet_ids('26_3')
cb2, sb2 = packet_ids('26_2')
items = [line.split()[0] for line in open(os.path.join(DATA, 'items_26_3.txt')) if line.strip()]

def bitset(body, p, byte_form):
    n, p = rvarint(body, p)
    size = 1 if byte_form else 8
    raw = body[p:p + n * size]; p += n * size
    return (int.from_bytes(raw, 'little') if byte_form else sum(struct.unpack('>Q', raw[i * 8:i * 8 + 8])[0] << (64 * i) for i in range(n))), p

def chunk(body, byte_form):
    x, z = struct.unpack('>ii', body[:8]); p = 8
    count, p = rvarint(body, p)
    for _ in range(count):
        kind, p = rvarint(body, p); n, p = rvarint(body, p); p += 8 * n
    size, p = rvarint(body, p); end = p + size; sections = []
    for _ in range(24):
        p += 4; bits = body[p]; p += 1
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
            raw = [(words[i // per] >> ((i % per) * bits)) & ((1 << bits) - 1) for i in range(4096)]
            blocks = [palette[v] for v in raw] if palette else raw
        p += 1; _, p = rvarint(body, p)
        sections.append(blocks)
    p = end
    entities, p = rvarint(body, p)
    sky, p = bitset(body, p, byte_form); block, p = bitset(body, p, byte_form)
    _, p = bitset(body, p, byte_form); _, p = bitset(body, p, byte_form)
    n, p = rvarint(body, p)
    for _ in range(n):
        length, p = rvarint(body, p); p += length
    n, p = rvarint(body, p); light = {}
    for bit in range(64):
        if (block >> bit) & 1:
            length, p = rvarint(body, p); light[bit - 1] = body[p:p + length]; p += length
    return x, z, sections, light, p == len(body), n == bin(block).count('1')

def level_at(light, section, index):
    data = light.get(section)
    return 0 if data is None else (data[index >> 1] >> ((index & 1) * 4)) & 15

BIN = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'build', 'bin')
open(os.path.join(BIN, 'players', 'lightholder.txt'), 'w').write('balance 100\nslot 0 minecraft:torch 1\nslot 2 minecraft:lantern 1\n')
a = Java('LightHolder', 777); a.join(); time.sleep(2.5)
with a.lock: chunks = [b for p, b in a.log if p == cb3['level_chunk_with_light']]; times = [b for p, b in a.log if p == cb3['set_time']]
parsed = [chunk(b, True) for b in chunks]
check('26.3 spawn chunks parse with block light', parsed and all(c[4] and c[5] for c in parsed), len(parsed))
lit = [c for c in parsed if c[3]]
check('spawn chunks carry block light', len(lit) > 0, '%d of %d chunks' % (len(lit), len(parsed)))
exact = wrong = 0
for x, z, sections, light, _, _ in parsed:
    for s, blocks in enumerate(sections):
        for i, state in enumerate(blocks):
            if state in glow:
                exact += level_at(light, s, i) >= glow[state]
                wrong += level_at(light, s, i) < glow[state]
check('every glowing block is lit at least at its own level', exact > 0 and wrong == 0, (exact, wrong))
dim = 0; spread = 0
for x, z, sections, light, _, _ in parsed:
    for s, blocks in enumerate(sections):
        for i, state in enumerate(blocks):
            if state in glow and glow[state] >= 10 and (i & 15) < 15 and blocks[i + 1] == 0:
                spread += level_at(light, s, i + 1) >= glow[state] - 1
                dim += 1
check('light spreads into the air next to it', dim and spread == dim, (spread, dim))
check('26.3 gets the time on join', times and struct.unpack('>q', times[-1][:8])[0] >= 0, len(times))
if times:
    p = 8; n, p = rvarint(times[-1], p); clock, p = rvarint(times[-1], p); total, p = rvarint(times[-1], p); partial, rate = struct.unpack('>ff', times[-1][p:p + 8])
    check('spawn is held at dusk (13000, rate 0)', (n, clock, total, rate) == (1, 0, 13000, 0.0), (n, clock, total, rate))

b = Java('LightWatch', 776); b.join(); time.sleep(1.5)
with b.lock: chunks2 = [x for p, x in b.log if p == cb2['level_chunk_with_light']]
parsed2 = [chunk(x, False) for x in chunks2]
check('26.2 spawn chunks parse with block light', parsed2 and all(c[4] and c[5] for c in parsed2) and any(c[3] for c in parsed2), len(parsed2))
c8 = Java('Light18', 47); c8.join(); time.sleep(0.8)
with c8.lock: t8 = [x for p, x in c8.log if p == 0x03]
check('1.8 gets a frozen dusk time', t8 and struct.unpack('>q', t8[-1][8:16])[0] == -13000, t8)

a.send(sb3['chat_command'], mstr('smp')); b.send(sb2['chat_command'], mstr('smp')); time.sleep(2.0)
with a.lock: spot = [x for p, x in a.log if p == cb3['player_position']][-1]
ax, ay, az = teleport_spot(spot)
def pos(x, y, z): return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))
def updates(client, pid, x, y, z):
    with client.lock: return [rvarint(m, 8)[0] for p, m in client.log if p == pid and m[:8] == pos(x, y, z)]
light26_2 = lambda level: 12566 - (15 - level) * 2
light26_3 = lambda level: 14413 - (15 - level) * 2
head = (int(ax // 1), int(ay // 1) + 1, int(az // 1))

a.send(sb3['set_carried_item'], struct.pack('>H', 1)); time.sleep(0.4)
for c in (a, b):
    with c.lock: c.log.clear()
a.send(sb3['set_carried_item'], struct.pack('>H', 0)); time.sleep(0.4)
check('holding a torch lights the holder (26.3 light 14)', updates(a, cb3['block_update'], *head)[-1:] == [light26_3(14)], updates(a, cb3['block_update'], *head))
check('and others see it (26.2 light 14)', updates(b, cb2['block_update'], *head)[-1:] == [light26_2(14)], updates(b, cb2['block_update'], *head))
a.move(ax + 3, ay, az, 0.0); time.sleep(0.4)
moved = (head[0] + 3, head[1], head[2])
check('walking moves the light and clears the old spot', updates(b, cb2['block_update'], *head)[-1:] == [0] and updates(b, cb2['block_update'], *moved)[-1:] == [light26_2(14)], (updates(b, cb2['block_update'], *head), updates(b, cb2['block_update'], *moved)))
a.send(sb3['set_carried_item'], struct.pack('>H', 1)); time.sleep(0.4)
check('switching to an empty hand turns it off', updates(b, cb2['block_update'], *moved)[-1:] == [0])
a.send(sb3['set_carried_item'], struct.pack('>H', 2)); time.sleep(0.4)
check('a lantern lights at 15', updates(b, cb2['block_update'], *moved)[-1:] == [light26_2(15)], updates(b, cb2['block_update'], *moved))
with a.lock: a.log.clear()
a.send(sb3['player_action'], varint(7) + pos(0, 0, 0) + b'\x00' + varint(0)); time.sleep(0.4)
with a.lock: swapped = [m for p, m in a.log if p == cb3['set_player_inventory']]
check('f moves it to the offhand and it still lights', updates(b, cb2['block_update'], *moved)[-1:] == [light26_2(15)] and any(m[0] == 40 and len(m) > 2 for m in swapped) and any(m[:2] == b'\x02\x00' for m in swapped), swapped)
a.close(); time.sleep(0.6)
check('leaving clears the light for others', updates(b, cb2['block_update'], *moved)[-1:] == [0])

time.sleep(5.2)
with b.lock: t = [x for p, x in b.log if p == cb2['set_time']]
if t:
    p = 8; _, p = rvarint(t[-1], p); _, p = rvarint(t[-1], p); total, p = rvarint(t[-1], p); _, rate = struct.unpack('>ff', t[-1][p:p + 8])
check('the smp runs a real day cycle', t and total >= 1000 and rate == 1.0, t and (total, rate))
b.close(); c8.close()
open(os.path.join(BIN, 'players', 'lightplacer.txt'), 'w').write('balance 100\nslot 0 minecraft:torch 4\n')
pl = Java('LightPlacer', 777); pl.join(); time.sleep(0.6)
pl.send(sb3['chat_command'], mstr('smp')); time.sleep(2.0)
with pl.lock: spot = [x for p, x in pl.log if p == cb3['player_position']][-1]
sx, sy, sz = [int(v // 1) for v in teleport_spot(spot)]
import random
tx, ty, tz = ((sx >> 4) << 4) + 15, sy + 40 + random.randint(0, 20), sz
pl.send(sb3['use_item_on'], varint(0) + pos(tx, ty - 1, tz) + varint(1) + struct.pack('>fff??', 0.5, 1.0, 0.5, False, False) + varint(5)); time.sleep(0.5)
pl.close(); time.sleep(0.3)
fresh = Java('LightFresh', 777); fresh.join(); time.sleep(0.6)
with fresh.lock: fresh.log.clear()
fresh.send(sb3['chat_command'], mstr('smp')); time.sleep(2.5)
with fresh.lock: smp_chunks = {(c[0], c[1]): c for c in (chunk(x, True) for p, x in fresh.log if p == cb3['level_chunk_with_light'])}
def smp_level(x, y, z):
    c = smp_chunks.get((x >> 4, z >> 4))
    return None if c is None else level_at(c[3], (y + 64) >> 4, ((y & 15) << 8) | ((z & 15) << 4) | (x & 15))
check('a placed torch is baked into smp block light', smp_level(tx, ty, tz) == 14, smp_level(tx, ty, tz))
check('and its light reaches into the next chunk', smp_level(tx + 1, ty, tz) == 13 and smp_level(tx + 3, ty, tz) == 11, (smp_level(tx + 1, ty, tz), smp_level(tx + 3, ty, tz)))
fresh.close()
print('failures:', fails)
