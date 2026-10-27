import time, sys, os, struct, json
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'data', 'java')
REPORT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'reference', 'gen_26_3', 'reports', 'blocks.json')
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
        solid = sum(1 for b in blocks if names[b] != 'minecraft:air')
        assert solid == non_air, ('non air count', solid, non_air)
        out.append(blocks)
    assert p == end
    return x, z, out

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

def table(name):
    out = {}
    for line in open(os.path.join(DATA, name)):
        parts = line.split()
        if parts: out[parts[0]] = int(parts[1])
    return out

items = [line.split()[0] for line in open(os.path.join(DATA, 'items_26_3.txt')) if line.strip()]
states_26_3, states_26_2 = table('blocks_26_3.txt'), table('blocks_26_2.txt')

def pos(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))

def pos18(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((y & 0xFFF) << 26) | (z & 0x3FFFFFF))

def updates(client, pid, packed):
    with client.lock:
        return [rvarint(b, 8)[0] for p, b in client.log if p == pid and b[:8] == packed]

def go_smp(client, command):
    client.send(command[0], mstr(command[1])); time.sleep(2.0)

cb3, sb3 = packet_ids('26_3')
cb2, sb2 = packet_ids('26_2')

b = Java('Mustinkycc', 777); b.join(); time.sleep(0.8)
w2 = Java('Watch262', 776); w2.join(); time.sleep(0.5)
w8 = Java('Watch18', 47); w8.join(); time.sleep(0.5)

with b.lock: b.log.clear()
b.send(sb3['use_item_on'], varint(0) + pos(8, 70, 8) + varint(1) + struct.pack('>fff??', 0.5, 1.0, 0.5, False, False) + varint(7)); time.sleep(0.4)
with b.lock: spawn_updates = [x for p, x in b.log if p == cb3['block_update']]
check('spawn stays protected (no block update, only the ack)', not spawn_updates, spawn_updates)

go_smp(b, (sb3['chat_command'], 'smp')); go_smp(w2, (sb2['chat_command'], 'smp')); go_smp(w8, (0x01, '/smp'))
b.send(sb3['chat_command'], mstr('gamemode creative')); time.sleep(0.3)
with b.lock: spot = [x for p, x in b.log if p == cb3['player_position']][-1]
sx, sy, sz = int(struct.unpack('>d', spot[1:9])[0] // 1), int(struct.unpack('>d', spot[9:17])[0]), int(struct.unpack('>d', spot[17:25])[0] // 1)

b.send(sb3['set_carried_item'], struct.pack('>H', 0))
b.send(sb3['set_creative_mode_slot'], struct.pack('>H', 36) + varint(1) + varint(items.index('minecraft:stone')) + varint(0) + varint(0))
stone = (sx + 2, sy + 3, sz)
b.send(sb3['use_item_on'], varint(0) + pos(stone[0], stone[1] - 1, stone[2]) + varint(1) + struct.pack('>fff??', 0.5, 1.0, 0.5, False, False) + varint(8)); time.sleep(0.5)
check('26.3 placer sees stone placed', updates(b, cb3['block_update'], pos(*stone)) == [states_26_3['minecraft:stone']], updates(b, cb3['block_update'], pos(*stone)))
check('26.2 player sees the stone', updates(w2, cb2['block_update'], pos(*stone)) == [states_26_2['minecraft:stone']], updates(w2, cb2['block_update'], pos(*stone)))
check('1.8 player sees the stone', updates(w8, 0x23, pos18(*stone)) == [1 << 4], updates(w8, 0x23, pos18(*stone)))

b.send(sb3['set_creative_mode_slot'], struct.pack('>H', 37) + varint(1) + varint(items.index('minecraft:glass')) + varint(0) + varint(0))
b.send(sb3['set_carried_item'], struct.pack('>H', 1))
glass = (sx - 2, sy + 3, sz + 1)
b.send(sb3['use_item_on'], varint(0) + pos(glass[0], glass[1] - 1, glass[2]) + varint(1) + struct.pack('>fff??', 0.5, 1.0, 0.5, False, False) + varint(9)); time.sleep(0.5)
check('glass (not a generated block) reaches 26.3', updates(b, cb3['block_update'], pos(*glass)) == [states_26_3['minecraft:glass']], updates(b, cb3['block_update'], pos(*glass)))
check('glass reaches 26.2', updates(w2, cb2['block_update'], pos(*glass)) == [states_26_2['minecraft:glass']], updates(w2, cb2['block_update'], pos(*glass)))
check('glass reaches 1.8', updates(w8, 0x23, pos18(*glass)) == [20 << 4], updates(w8, 0x23, pos18(*glass)))

b.send(sb3['player_action'], varint(0) + pos(*stone) + b'\x01' + varint(10)); time.sleep(0.5)
check('creative break turns the stone to air for everyone', updates(b, cb3['block_update'], pos(*stone))[-1:] == [0] and updates(w2, cb2['block_update'], pos(*stone))[-1:] == [0] and updates(w8, 0x23, pos18(*stone))[-1:] == [0], None)

w8.send(0x08, pos18(sx + 1, sy + 2, sz - 2) + b'\x01' + struct.pack('>hbhb', 1, 1, 0, 0) + b'\x08\x10\x08'); time.sleep(0.5)
check('1.8 placing stone reaches 26.3', updates(b, cb3['block_update'], pos(sx + 1, sy + 3, sz - 2)) == [states_26_3['minecraft:stone']], updates(b, cb3['block_update'], pos(sx + 1, sy + 3, sz - 2)))

f = Java('FreshBuild', 777); f.join(); time.sleep(0.5)
with f.lock: f.log.clear()
go_smp(f, (sb3['chat_command'], 'smp'))
with f.lock: chunks = [x for p, x in f.log if p == cb3['level_chunk_with_light']]
found = {}
for body in chunks:
    x, z, sections = sections_of(body)
    found[(x, z)] = sections
def block_at(x, y, z):
    return found[(x >> 4, z >> 4)][(y + 64) >> 4][(y & 15) * 256 + (z & 15) * 16 + (x & 15)]
check('a new player gets the glass baked into the chunk', block_at(*glass) == states_26_3['minecraft:glass'], block_at(*glass))
check('and the broken stone stays air', block_at(*stone) == 0, block_at(*stone))
check('and the 1.8 stone', block_at(sx + 1, sy + 3, sz - 2) == states_26_3['minecraft:stone'], block_at(sx + 1, sy + 3, sz - 2))

for x in (b, w2, w8, f): x.close()
print('failures:', fails)
