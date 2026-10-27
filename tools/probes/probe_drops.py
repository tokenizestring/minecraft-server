import time, sys, os, struct
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

cb3, sb3 = packet_ids('26_3')
cb2, sb2 = packet_ids('26_2')

def pos(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))

def bodies(client, pid):
    with client.lock:
        return [b for p, b in client.log if p == pid]

def items_added(client, cb, kind):
    return [b for b in bodies(client, cb['add_entity']) if rvarint(b, rvarint(b, 0)[1] + 16)[0] == kind]

def taken(client, pid):
    return [(rvarint(b, 0)[0], rvarint(b, rvarint(b, 0)[1])[0]) for b in bodies(client, pid)]

m = Java('DropMiner', 777); m.join(); time.sleep(0.6)
with m.lock: miner = struct.unpack('>i', [b for p, b in m.log if p == cb3['login']][0][:4])[0]
w2 = Java('DropWatch262', 776); w2.join(); time.sleep(0.4)
w8 = Java('DropWatch18', 47); w8.join(); time.sleep(0.4)
for client, command in ((m, (sb3['chat_command'], mstr('smp'))), (w2, (sb2['chat_command'], mstr('smp'))), (w8, (0x01, mstr('/smp')))):
    client.send(*command)
time.sleep(2.5)
with m.lock: spot = [x for p, x in m.log if p == cb3['player_position']][-1]
sx, sy, sz = int(struct.unpack('>d', spot[1:9])[0] // 1), int(struct.unpack('>d', spot[9:17])[0]), int(struct.unpack('>d', spot[17:25])[0] // 1)
import random
sx += random.randint(-40, 40); sz += random.randint(-40, 40)
m.move(sx + 0.5, sy, sz + 0.5, 0.0)
w2.move(sx + 8.5, sy, sz - 6.5, 0.0); w8.move(sx + 8.5, sy, sz + 0.5, 0.0); time.sleep(0.6)

for client in (m, w2, w8):
    with client.lock: client.log.clear()
m.send(sb3['set_carried_item'], struct.pack('>H', 0))
for depth in range(1, 20):
    m.move(sx + 0.5, sy - depth + 1, sz + 0.5, 0.0)
    m.send(sb3['player_action'], varint(0) + pos(sx, sy - depth, sz) + b'\x01' + varint(depth * 2))
    m.send(sb3['player_action'], varint(3) + pos(sx, sy - depth, sz) + b'\x01' + varint(depth * 2 + 1)); time.sleep(0.25)
    if items_added(m, cb3, 72): break
spawned = items_added(m, cb3, 72)
check('breaking a block spawns an item entity for 26.3', len(spawned) == 1, depth)
drop_id = rvarint(spawned[0], 0)[0] if spawned else None
data = [b for b in bodies(m, cb3['set_entity_data']) if rvarint(b, 0)[0] == drop_id]
check('26.3 gets the item stack metadata (index 8, type 7)', data and data[0][rvarint(data[0], 0)[1]:][:2] == b'\x08\x07', data)
check('26.2 sees the same drop as item type 71', any(rvarint(b, 0)[0] == drop_id for b in items_added(w2, cb2, 71)))
check('1.8 sees it as an item object', any(rvarint(b, 0)[0] == drop_id and b[rvarint(b, 0)[1]] == 2 for b in bodies(w8, 0x0E)) and any(rvarint(b, 0)[0] == drop_id for b in bodies(w8, 0x1C)))
time.sleep(1.5)
check('the miner picks it up (take item)', (drop_id, miner) in taken(m, cb3['take_item_entity']), taken(m, cb3['take_item_entity']))
check('26.2 and 1.8 see the pickup', (drop_id, miner) in taken(w2, cb2['take_item_entity']) and (drop_id, miner) in taken(w8, 0x0D))
check('the drop is removed for everyone', any(drop_id in [rvarint(b, rvarint(b, 0)[1])[0]] for b in bodies(m, cb3['remove_entities'])) and any(rvarint(b, rvarint(b, 0)[1])[0] == drop_id for b in bodies(w8, 0x13)))
inv = bodies(m, cb3['container_set_content'])
check('and it lands in the hotbar', len(inv) >= 1)

with m.lock: m.log.clear()
for _ in range(5):
    m.send(sb3['player_action'], varint(0) + pos(sx + 2, sy - 1, sz) + b'\x01' + varint(90))
    m.send(sb3['player_action'], varint(2) + pos(sx + 2, sy - 1, sz) + b'\x01' + varint(91)); time.sleep(0.05)
time.sleep(0.4)
with m.lock: broke = [b for p, b in m.log if p == cb3['block_update'] and b[:8] == pos(sx + 2, sy - 1, sz) and rvarint(b, 8)[0] == 0]
check('spam clicking (start then abort) breaks nothing in survival', not broke and not items_added(m, cb3, 72), len(broke))

for client in (m, w2, w8):
    with client.lock: client.log.clear()
m.send(sb3['player_action'], varint(5) + pos(0, 0, 0) + b'\x00' + varint(0)); time.sleep(0.3)
tossed = items_added(w2, cb2, 71)
check('pressing q throws the item where others see it', len(tossed) == 1)
toss_id = rvarint(tossed[0], 0)[0] if tossed else None
moves = [b for b in bodies(w2, cb2['move_entity_pos']) if rvarint(b, 0)[0] == toss_id]
moves8 = [b for b in bodies(w8, 0x15) + bodies(w8, 0x18) if rvarint(b, 0)[0] == toss_id]
check('the throw flies (26.2 and 1.8 get moves)', len(moves) >= 3 and len(moves8) >= 3, (len(moves), len(moves8)))
def packed(b):
    eid, p = rvarint(b, 0); props, p = rvarint(b, p)
    return props <= 1 and len(b) - p == 6
moves3 = [b for b in bodies(m, cb3['move_entity_pos']) if rvarint(b, 0)[0] == toss_id]
check('26.3 moves use the packed layout (id, flags, 3 shorts)', len(moves3) >= 3 and all(packed(b) for b in moves3), [b.hex() for b in moves3[:2]])
check('the thrower cannot grab it straight back', (toss_id, miner) not in taken(m, cb3['take_item_entity']))
for _ in range(40):
    time.sleep(0.5)
    if (toss_id, miner) in taken(m, cb3['take_item_entity']): break
check('after resting a while it flies back to the thrower', (toss_id, miner) in taken(m, cb3['take_item_entity']), taken(m, cb3['take_item_entity']))

for client in (m, w2, w8):
    with client.lock: client.log.clear()
w8.send(0x09, struct.pack('>H', 0))
for depth in range(1, 20):
    w8.move(sx + 8.5, sy - depth + 1, sz + 0.5, 0.0)
    w8.send(0x07, b'\x00' + struct.pack('>Q', (((sx + 8) & 0x3FFFFFF) << 38) | (((sy - depth) & 0xFFF) << 26) | (sz & 0x3FFFFFF)) + b'\x01')
    w8.send(0x07, b'\x02' + struct.pack('>Q', (((sx + 8) & 0x3FFFFFF) << 38) | (((sy - depth) & 0xFFF) << 26) | (sz & 0x3FFFFFF)) + b'\x01'); time.sleep(0.25)
    if bodies(w8, 0x0D): break
time.sleep(1.0)
got8 = [b for b in bodies(w8, 0x30) if b[0] == 0]
check('1.8 mining drops and collects into the 1.8 inventory', bodies(w8, 0x0D) and got8, depth)
with m.lock: m.log.clear()
w8.send(0x07, b'\x04' + bytes(8) + b'\x00'); time.sleep(0.4)
check('1.8 q throws an item 26.3 can see', len(items_added(m, cb3, 72)) == 1)

for client in (m, w2, w8):
    with client.lock: client.log.clear()
m.send(sb3['chat_command'], mstr('spawn')); time.sleep(0.6)
m.send(sb3['player_action'], varint(4) + pos(0, 0, 0) + b'\x00' + varint(0)); time.sleep(0.4)
check('q in spawn keeps the item (no entity, hotbar resent)', not items_added(m, cb3, 72) and bodies(m, cb3['container_set_content']))
for client in (m, w2, w8): client.close()
print('failures:', fails)
