import time, sys, os, struct, json, re
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

LOG = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'build', 'bin', 'server.log')
ITEMS = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'data', 'java', 'items_%s.txt')

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

def npc_entity():
    found = re.findall(r"\[npc\]\(load\) 'Servers' entity (\d+)", open(LOG, encoding='utf-8', errors='replace').read())
    return int(found[-1])

def item_id(tag, name):
    return open(ITEMS % tag).read().split('\n').index(name)

def rstr(b, p):
    n, p = rvarint(b, p); return b[p:p + n], p + n

def nbt_string(b, p):
    n = struct.unpack('>H', b[p:p + 2])[0]; return b[p + 2:p + 2 + n].decode('utf-8', 'replace'), p + 2 + n

def plain(b, p):
    assert b[p] == 0x0A; p += 1; fields = {}
    while b[p] != 0:
        kind = b[p]; key, p = nbt_string(b, p + 1)
        if kind == 8: fields[key], p = nbt_string(b, p)
        elif kind == 1: fields[key] = b[p]; p += 1
    return fields, p + 1

def modern_slots(body):
    window, p = rvarint(body, 0); state, p = rvarint(body, p); count, p = rvarint(body, p); slots = {}
    for slot in range(count):
        n, p = rvarint(body, p)
        if n:
            item, p = rvarint(body, p); added, p = rvarint(body, p); removed, p = rvarint(body, p); name = None; lore = []
            for _ in range(added):
                kind, p = rvarint(body, p)
                if kind == 9: fields, p = plain(body, p); name = fields['text']; assert fields['italic'] == 0
                elif kind == 11:
                    lines, p = rvarint(body, p)
                    for _ in range(lines):
                        fields, p = plain(body, p); lore.append(fields['text'])
            slots[slot] = (item, name, lore)
    carried, p = rvarint(body, p)
    assert p == len(body) and carried == 0, ('content tail', p, len(body))
    return window, count, slots

entity = npc_entity()
print('npc entity', entity)
for protocol, tag, hit in ((777, '26_3', 'attack'), (776, '26_2', 'interact')):
    cb, sb = packet_ids(tag)
    c = Java('Npc' + tag, protocol); c.join(); time.sleep(1.5)
    with c.lock: log = list(c.log)
    adds = [rvarint(b, 0)[0] for p, b in log if p == cb['add_entity']]
    infos = [b for p, b in log if p == cb['player_info_update'] and b'Servers' in b]
    check('%s sees the npc entity' % tag, entity in adds, adds)
    check('%s npc kept out of the tab list' % tag, infos and infos[0][-8] == 0, [i.hex()[-20:] for i in infos])
    with c.lock: c.log.clear()
    if hit == 'attack': c.send(sb['attack'], varint(entity))
    else: c.send(sb['interact'], varint(entity) + varint(0) + b'\x00' + b'\x00')
    time.sleep(0.6)
    with c.lock: log = list(c.log)
    screens = [b for p, b in log if p == cb['open_screen']]
    contents = [b for p, b in log if p == cb['container_set_content']]
    check('%s %s opens the menu' % (tag, hit), len(screens) == 1 and screens[0][:2] == b'\x07\x02', [s.hex()[:20] for s in screens])
    window, count, slots = modern_slots(contents[0])
    want = {11: item_id(tag, 'minecraft:grass_block'), 13: item_id(tag, 'minecraft:diamond_sword'), 15: item_id(tag, 'minecraft:compass')}
    check('%s menu has 27 slots with the right items' % tag, count == 27 and {k: v[0] for k, v in slots.items()} == want, slots)
    check('%s item names and lore decoded' % tag, slots[11][1].endswith('Survival') and len(slots[11][2]) == 2, slots[11])
    with c.lock: c.log.clear()
    c.send(sb['container_click'], varint(7) + varint(0) + struct.pack('>hb', 11, 0) + varint(0) + varint(0) + b'\x00'); time.sleep(0.6)
    with c.lock: log = list(c.log)
    closes = [b for p, b in log if p == cb['container_close']]
    notes = [b for p, b in log if p == cb['system_chat']]
    check('%s clicking survival closes the menu' % tag, closes == [b'\x07'], closes)
    check('%s and runs its action' % tag, any(b'survival is coming soon' in n for n in notes), notes)
    check('%s menu restored and cursor cleared' % tag, any(p == cb['container_set_content'] for p, b in log) and any(p == cb['set_cursor_item'] and b == b'\x00' for p, b in log))
    c.close()

c = Java('Npc18', 47); c.join(); time.sleep(1.0)
with c.lock:
    spawned = [rvarint(b, 0)[0] for p, b in c.log if p == 0x0C]
    removes = [b for p, b in c.log if p == 0x38 and b[0] == 4]
check('1.8 sees the npc and it leaves the tab list', entity in spawned and removes, (spawned, len(removes)))
with c.lock: c.log.clear()
c.send(0x02, varint(entity) + varint(1)); time.sleep(0.6)
with c.lock: log = list(c.log)
opens = [b for p, b in log if p == 0x2D]
items = [b for p, b in log if p == 0x30]
check('1.8 hitting the npc opens a 27 slot chest', opens and opens[0][0] == 7 and opens[0][-1] == 27, [o.hex() for o in opens])
window = items[0]; q = 3; got = {}
for slot in range(27):
    item = struct.unpack('>h', window[q:q + 2])[0]; q += 2
    if item != -1:
        count, damage = window[q], struct.unpack('>h', window[q + 1:q + 3])[0]; q += 3
        assert window[q] == 0x0A; q += 3; depth = 1; name = None
        assert window[q] == 0x0A; display, q = nbt_string(window, q + 1)
        while window[q] != 0:
            kind = window[q]; key, q = nbt_string(window, q + 1)
            if kind == 8: name, q = nbt_string(window, q)
            elif kind == 9:
                q += 1; n = struct.unpack('>i', window[q:q + 4])[0]; q += 4
                for _ in range(n): _, q = nbt_string(window, q)
        q += 2
        got[slot] = (item, name)
check('1.8 chest items are grass, diamond sword, compass', {k: v[0] for k, v in got.items()} == {11: 2, 13: 276, 15: 345} and q == len(window), got)
with c.lock: c.log.clear()
c.send(0x0E, b'\x07' + struct.pack('>h', 13) + b'\x00' + struct.pack('>h', 5) + b'\x00' + struct.pack('>h', -1)); time.sleep(0.6)
with c.lock: log = list(c.log)
check('1.8 click is refused, chest closed and action run', any(p == 0x32 and b == b'\x07\x00\x05\x00' for p, b in log) and any(p == 0x2E for p, b in log) and any(p == 0x02 and b'pvp is coming soon' in b for p, b in log), [(hex(p), b[:24]) for p, b in log])
c.close()

d = Bedrock('NpcBedrock'); d.join(); time.sleep(1.5)
names = [b for b in d.packets(12) if b'Servers' in b]
check('bedrock sees the npc', len(names) == 1)
d.client.game([bp.uvarint(30) + b'\x00' + bp.uvarint(3) + bp.uvarint(0) + bp.uvarint(entity) + bp.uvarint(1) + b'\x00' * 20]); time.sleep(0.8)
forms = d.packets(100)
check('bedrock hitting the npc opens a form', len(forms) == 1)
form_id, p = rvarint(forms[0], 0); text, p = rstr(forms[0], p)
form = json.loads(text)
check('bedrock form has the 3 servers', [b['text'].split('\n')[0][2:] for b in form['buttons']] == ['Survival', 'PvP', 'Lobby'], form)
d.client.game([bp.uvarint(101) + bp.uvarint(form_id) + b'\x01' + bp.uvarint(2) + b'1\n' + b'\x00']); time.sleep(0.8)
tips = [b for b in d.packets(9) if b[:3] == b'\x00\x00\x05']
check('bedrock choosing pvp runs its action', any(b'pvp is coming soon' in t for t in tips), tips)
d.close()
print('failures:', fails)
