import time, sys, os, struct
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

BIN = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'build', 'bin')
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'data', 'java')

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

cb3, sb3 = packet_ids('26_3')
cb2, sb2 = packet_ids('26_2')
items3 = [line.split()[0] for line in open(os.path.join(DATA, 'items_26_3.txt')) if line.strip()]
MOBS3 = {30: 'cow', 101: 'pig', 114: 'sheep', 26: 'chicken'}
MOBS2 = {30: 'cow', 100: 'pig', 111: 'sheep', 26: 'chicken'}

def bodies(client, pid):
    with client.lock:
        return [b for p, b in client.log if p == pid]

def healths(client, pid):
    out = []
    for b in bodies(client, pid):
        health = struct.unpack('>f', b[:4])[0]; food, p = rvarint(b, 4); sat = struct.unpack('>f', b[p:p + 4])[0]
        out.append((round(health, 1), food, round(sat, 1)))
    return out

def mobs_seen(client, cb, table):
    out = {}
    for b in bodies(client, cb['add_entity']):
        eid, p = rvarint(b, 0); p += 16; kind, p = rvarint(b, p)
        if kind in table:
            out[eid] = (table[kind],) + struct.unpack('>ddd', b[p:p + 24])
    return out

def sound_owner(m):
    _, p = rvarint(m, 0); _, p = rvarint(m, p); owner, p = rvarint(m, p)
    return owner

def notes(client):
    return bodies(client, cb3['system_chat'])

def move(client, x, y, z, ground):
    client.send(sb3['move_player_pos_rot'], struct.pack('>dddffB', x, y, z, 0.0, 0.0, 1 if ground else 0))

open(os.path.join(BIN, 'players', 'mobhunter.txt'), 'w').write('balance 100\nvitals 20.0 10 0.0\nslot 0 minecraft:diamond_sword 1\nslot 1 minecraft:cooked_beef 3\n')
a = Java('MobHunter', 777); a.join(); time.sleep(0.8)
with a.lock: me = struct.unpack('>i', [b for p, b in a.log if p == cb3['login']][0][:4])[0]
check('spawn shows frozen full health and food', healths(a, cb3['set_health'])[-1:] == [(20.0, 20, 5.0)], healths(a, cb3['set_health']))
with a.lock: a.log.clear()
a.send(sb3['chat_command'], mstr('smp')); time.sleep(2.0)
check('the smp shows the real saved values', healths(a, cb3['set_health'])[-1:] == [(20.0, 10, 0.0)], healths(a, cb3['set_health']))
with a.lock: spot = [x for p, x in a.log if p == cb3['player_position']][-1]
sx, sy, sz = teleport_spot(spot)

b = Java('MobWatch', 776); b.join(); time.sleep(0.4)
b.send(sb2['chat_command'], mstr('smp')); time.sleep(1.0)
c = Java('MobOld', 47); c.join(); time.sleep(0.4)
c.send(0x01, mstr('/smp')); time.sleep(1.0)
seen = {}
for _ in range(30):
    time.sleep(0.5)
    seen = mobs_seen(a, cb3, MOBS3)
    if len(seen) >= 6: break
check('animals spawn around the player (26.3)', len(seen) >= 4, (len(seen), sorted(set(v[0] for v in seen.values()))))
check('26.2 sees animals too', len(mobs_seen(b, cb2, MOBS2)) >= 2, len(mobs_seen(b, cb2, MOBS2)))
with c.lock: old = [x for p, x in c.log if p == 0x0F and x[rvarint(x, 0)[1]] in (90, 91, 92, 93)]
check('1.8 gets spawn mob packets', len(old) >= 2, len(old))
time.sleep(4.0)
moves = [m for m in bodies(a, cb3['move_entity_pos']) if rvarint(m, 0)[0] in seen]
def packed(m):
    _, p = rvarint(m, 0); flags, p = rvarint(m, p); return flags <= 1 and len(m) - p == 6
check('animals wander (packed 26.3 moves)', len(moves) >= 1 and all(packed(m) for m in moves), len(moves))

light_src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'probe_light.py')).read()
exec('def bitset' + light_src.split('def bitset', 1)[1].split('def level_at', 1)[0])
see_through = set()
for line in open(os.path.join(DATA, 'light.txt')):
    state, _, opacity = map(int, line.split())
    if opacity == 0: see_through.add(state)
see_through.add(0)
with a.lock: raw_chunks = [x for p, x in a.log if p == cb3['level_chunk_with_light']]
blocks_at = {}
for body in raw_chunks:
    cx, cz, sections, _, _, _ = chunk(body, True)
    blocks_at[(cx, cz)] = sections
def block(x, y, z):
    s = blocks_at.get((x >> 4, z >> 4))
    return None if s is None else s[(y + 64) >> 4][((y & 15) << 8) | ((z & 15) << 4) | (x & 15)]
spots = []
for eid in seen:
    _, x, y, z = seen[eid]; spot = [x, y, z]
    with a.lock: log = list(a.log)
    for p, m in log:
        if p == cb3['move_entity_pos'] and rvarint(m, 0)[0] == eid:
            _, q = rvarint(m, 0); _, q = rvarint(m, q); dx, dy, dz = struct.unpack('>hhh', m[q:q + 6]); spot = [spot[0] + dx / 4096, spot[1] + dy / 4096, spot[2] + dz / 4096]
            spots.append(tuple(spot))
inside = [s for s in spots if block(int(s[0] // 1), int(round(s[1])), int(s[2] // 1)) not in see_through | {None} or block(int(s[0] // 1), int(round(s[1])) + 1, int(s[2] // 1)) not in see_through | {None}]
check('animals never walk inside solid blocks', spots and not inside, (len(spots), inside[:3]))
herd_spot = spots[-1] if spots else seen[next(iter(seen))][1:]
before = len(bodies(a, cb3['sound_entity']))
move(a, herd_spot[0], herd_spot[1], herd_spot[2], True)
for _ in range(20):
    time.sleep(0.5)
    sounds = [m for m in bodies(a, cb3['sound_entity'])[before:] if sound_owner(m) in seen]
    if sounds: break
check('animals make ambient sounds', len(sounds) >= 1, len(sounds))

def where(eid):
    _, x, y, z = seen[eid]; spot = [x, y, z]
    with a.lock: log = list(a.log)
    for p, m in log:
        if p == cb3['add_entity'] and rvarint(m, 0)[0] == eid:
            _, q = rvarint(m, 0); q += 16; _, q = rvarint(m, q); spot = list(struct.unpack('>ddd', m[q:q + 24]))
        elif p == cb3['move_entity_pos'] and rvarint(m, 0)[0] == eid:
            _, q = rvarint(m, 0); _, q = rvarint(m, q); dx, dy, dz = struct.unpack('>hhh', m[q:q + 6]); spot = [spot[0] + dx / 4096, spot[1] + dy / 4096, spot[2] + dz / 4096]
        elif p == cb3['teleport_entity'] and rvarint(m, 0)[0] == eid:
            _, q = rvarint(m, 0); spot = list(struct.unpack('>ddd', m[q:q + 24]))
    return spot

move(a, sx, sy, sz, True); time.sleep(1.2)
target = next((eid for eid, v in seen.items() if v[0] in ('cow', 'pig')), next(iter(seen), None))
if target:
    eid, kind = target, seen[target][0]
    for hit in range(4):
        x, y, z = where(eid)
        move(a, x, y, z, True); time.sleep(0.05)
        a.send(sb3['attack'], varint(eid)); time.sleep(0.7)
        with a.lock: dead = [m for p, m in a.log if p == cb3['entity_event'] and struct.unpack('>i', m[:4])[0] == eid and m[4] == 3]
        if dead: break
    hurt = [m for m in bodies(a, cb3['hurt_animation']) if rvarint(m, 0)[0] == eid]
    check('hitting an animal plays the hurt flash', len(hurt) >= 1, (kind, len(hurt)))
    check('and hurt and death sounds', len([m for m in bodies(a, cb3['sound_entity']) if sound_owner(m) == eid]) >= 2)
    check('a diamond sword kills it in a couple of hits', bool(dead), (kind, hit + 1))
    time.sleep(1.5)
    items = [m for m in bodies(a, cb3['add_entity']) if rvarint(m, rvarint(m, 0)[1] + 16)[0] == 72]
    check('it drops items on death', len(items) >= 1, len(items))
    removed = any(eid in [rvarint(m, rvarint(m, 0)[1])[0]] for m in bodies(a, cb3['remove_entities']))
    check('the body is removed after the death animation', removed)

move(a, sx, sy, sz, True); time.sleep(0.3)
with a.lock: a.log.clear()
a.send(sb3['set_carried_item'], struct.pack('>H', 1))
a.send(sb3['use_item'], varint(0) + varint(9) + struct.pack('>ff', 0.0, 0.0)); time.sleep(2.2)
eaten = [m for m in bodies(a, cb3['entity_event']) if struct.unpack('>i', m[:4])[0] == me and m[4] == 9]
check('eating cooked beef finishes after 32 ticks', bool(eaten), len(eaten))
check('with a burp', any(sound_owner(m) == me for m in bodies(a, cb3['sound_entity'])))
check('food goes up by 8', any(h[1] >= 17 for h in healths(a, cb3['set_health'])), healths(a, cb3['set_health']))
slot = [m for m in bodies(a, cb3['set_player_inventory']) if m[0] == 1]
check('one beef is used up', slot and slot[-1][1] == 2, slot)

with a.lock: a.log.clear()
for step in range(12):
    move(a, sx, sy + step, sz, False); time.sleep(0.03)
move(a, sx, sy, sz, True); time.sleep(0.4)
hp = healths(a, cb3['set_health'])
check('a 11 block fall hurts', hp and hp[-1][0] < 20.0, hp)
check('with fall and hurt sounds', len([m for m in bodies(a, cb3['sound_entity']) if sound_owner(m) == me]) >= 2)

with a.lock: a.log.clear()
for step in range(60):
    move(a, sx, sy + step, sz, False); time.sleep(0.01)
move(a, sx, sy, sz, True); time.sleep(0.6)
check('a huge fall kills and respawns you', any(b'you died' in n for n in notes(a)) and healths(a, cb3['set_health'])[-1:] == [(20.0, 20, 5.0)], (notes(a)[-1:], healths(a, cb3['set_health'])[-2:]))
dropped = [m for m in bodies(a, cb3['add_entity']) if rvarint(m, rvarint(m, 0)[1] + 16)[0] == 72]
check('your items drop where you died', len(dropped) >= 1, len(dropped))

with a.lock: a.log.clear()
a.send(sb3['chat_command'], mstr('spawn')); time.sleep(1.0)
check('spawn freezes the bars at full again', healths(a, cb3['set_health'])[-1:] == [(20.0, 20, 5.0)], healths(a, cb3['set_health']))
with a.lock: a.log.clear()
for step in range(30):
    move(a, 12.5, 60 + step, 7.5, False); time.sleep(0.01)
move(a, 12.5, 60.0, 7.5, True); time.sleep(0.5)
check('falling in spawn does nothing', not healths(a, cb3['set_health']), healths(a, cb3['set_health']))
for x in (a, b, c): x.close()
time.sleep(0.5)
check('vitals are saved in the profile', 'vitals ' in open(os.path.join(BIN, 'players', 'mobhunter.txt')).read())
print('failures:', fails)
