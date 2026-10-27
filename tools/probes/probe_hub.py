import time, sys, os, struct, json, re
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'data', 'java')
BIN = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'build', 'bin')

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

items3 = [line.split()[0] for line in open(os.path.join(DATA, 'items_26_3.txt')) if line.strip()]
items2 = [line.split()[0] for line in open(os.path.join(DATA, 'items_26_2.txt')) if line.strip()]
cb3, sb3 = packet_ids('26_3')
cb2, sb2 = packet_ids('26_2')

def text_of(body, p):
    assert body[p] == 0x0A; p += 1
    while body[p]:
        tag = body[p]; n = struct.unpack('>H', body[p + 1:p + 3])[0]; p += 3 + n
        p += 1 if tag == 1 else 2 + struct.unpack('>H', body[p:p + 2])[0]
    return p + 1

def plain(body, p):
    tag = body[p]
    if tag == 8:
        n = struct.unpack('>H', body[p + 1:p + 3])[0]; return body[p + 3:p + 3 + n].decode('utf-8', 'replace'), p + 3 + n
    start, end, text = p, text_of(body, p), ''
    q = start + 1
    while body[q]:
        tag = body[q]; n = struct.unpack('>H', body[q + 1:q + 3])[0]; key = body[q + 3:q + 3 + n]; q += 3 + n
        if tag == 8:
            m = struct.unpack('>H', body[q:q + 2])[0]; text = body[q + 2:q + 2 + m].decode('utf-8', 'replace') if key == b'text' else text; q += 2 + m
        else:
            q += 1
    return text, end

def slots_of(body):
    window, p = rvarint(body, 0); state, p = rvarint(body, p); n, p = rvarint(body, p); out = []
    for _ in range(n + 1):
        count, p = rvarint(body, p)
        if count:
            item, p = rvarint(body, p); added, p = rvarint(body, p); removed, p = rvarint(body, p); name = ''
            for _ in range(added):
                kind, p = rvarint(body, p)
                lines, p = rvarint(body, p) if kind == 11 else (1, p)
                for _ in range(lines):
                    text, p = plain(body, p); name = name or text
            out.append((item, count, name))
        else:
            out.append(None)
    return window, out[:-1], out[-1]

def contents(client, cb, window):
    with client.lock:
        return [slots_of(b) for p, b in client.log if p == cb['container_set_content'] and rvarint(b, 0)[0] == window]

def notes(client, cb):
    with client.lock:
        return [b for p, b in client.log if p == cb['system_chat']]

def clear(*clients):
    for c in clients:
        with c.lock: c.log.clear()

def use(client, sb):
    client.send(sb['use_item'], varint(0) + varint(7) + struct.pack('>ff', 0.0, 0.0)); time.sleep(0.4)

def screens(client, cb):
    with client.lock:
        return [b for p, b in client.log if p == cb['open_screen']]

def click(client, sb, window, slot, button=0):
    client.send(sb['container_click'], varint(window) + varint(0) + struct.pack('>hb', slot, button) + varint(0) + varint(0) + b'\x00'); time.sleep(0.4)

for name in ('hubone', 'hubtwo'):
    path = os.path.join(BIN, 'players', name + '.txt')
    if os.path.exists(path): os.remove(path)
open(os.path.join(BIN, 'players', 'hubone.txt'), 'w').write('balance 100\nslot 0 minecraft:dirt 5\n')

a = Java('HubOne', 777); a.join(); time.sleep(0.8)
inv = contents(a, cb3, 0)
hot = inv[-1][1][36:45] if inv else []
check('spawn hotbar is compass, book, clock', hot and items3[hot[0][0]] == 'minecraft:compass' and items3[hot[4][0]] == 'minecraft:book' and items3[hot[8][0]] == 'minecraft:clock', hot)
check('hub items carry names', hot and 'Games' in hot[0][2] and 'Friends' in hot[4][2] and 'Info' in hot[8][2], hot and [h and h[2] for h in hot])
check('smp dirt is not shown in spawn', not any(s and items3[s[0]] == 'minecraft:dirt' for s in inv[-1][1]) if inv else False)

clear(a)
a.send(sb3['set_carried_item'], struct.pack('>H', 0)); use(a, sb3)
menu = contents(a, cb3, 7)
check('right click compass opens the games menu', screens(a, cb3) and rvarint(screens(a, cb3)[-1], 0)[0] == 7 and menu and menu[-1][1][11] and items3[menu[-1][1][11][0]] == 'minecraft:grass_block', menu and menu[-1][1][11])
clear(a)
a.send(sb3['set_carried_item'], struct.pack('>H', 8)); use(a, sb3)
menu = contents(a, cb3, 7)
check('right click clock opens the info menu', menu and menu[-1][1][11] and items3[menu[-1][1][11][0]] == 'minecraft:book', menu and menu[-1][1][11])

clear(a)
click(a, sb3, 0, 36)
check('moving a hub item is undone', len(contents(a, cb3, 0)) == 1 and contents(a, cb3, 0)[-1][1][36] and items3[contents(a, cb3, 0)[-1][1][36][0]] == 'minecraft:compass')
clear(a)
a.send(sb3['set_carried_item'], struct.pack('>H', 0))
for status in (4, 5):
    clear(a)
    a.send(sb3['player_action'], varint(status) + struct.pack('>Q', 0) + b'\x00' + varint(0)); time.sleep(0.4)
    with a.lock: spawned = [b for p, b in a.log if p == cb3['add_entity']]
    resent = contents(a, cb3, 0)
    check('q (status %d) in spawn throws nothing and resends the hub hotbar' % status, resent and resent[-1][1][36] and items3[resent[-1][1][36][0]] == 'minecraft:compass' and not spawned, resent and resent[-1][1][36])
clear(a)
click(a, sb3, 0, -999)
check('dropping outside the window in spawn is undone', contents(a, cb3, 0) and items3[contents(a, cb3, 0)[-1][1][36][0]] == 'minecraft:compass')
clear(a)
a.send(sb3['player_action'], varint(7) + struct.pack('>Q', 0) + b'\x00' + varint(0)); time.sleep(0.4)
check('f in spawn swaps nothing', contents(a, cb3, 0) and contents(a, cb3, 0)[-1][1][45] is None)
clear(a)
a.send(sb3['chat_command'], mstr('sell 5')); time.sleep(0.4)
check('/sell in spawn is refused', any(b'sell from there' in n for n in notes(a, cb3)), notes(a, cb3))

clear(a)
a.send(sb3['set_carried_item'], struct.pack('>H', 0)); use(a, sb3)
click(a, sb3, 7, 11); time.sleep(1.5)
smp = contents(a, cb3, 0)
check('games menu joins the smp', any(b'welcome to smp' in n for n in notes(a, cb3)), notes(a, cb3))
check('the smp inventory comes back (dirt in slot 1)', smp and smp[-1][1][36] and items3[smp[-1][1][36][0]] == 'minecraft:dirt' and smp[-1][1][36][1] == 5, smp and smp[-1][1][36])
clear(a)
use(a, sb3)
check('right click in the smp opens nothing', not screens(a, cb3))
a.send(sb3['chat_command'], mstr('spawn')); time.sleep(1.0)
back = contents(a, cb3, 0)
check('back at spawn the hub hotbar returns', back and back[-1][1][36] and items3[back[-1][1][36][0]] == 'minecraft:compass', back and back[-1][1][36])

clear(a)
a.send(sb3['set_carried_item'], struct.pack('>H', 0)); use(a, sb3)
click(a, sb3, 7, 13); time.sleep(1.5)
with a.lock: jumps = [x for p, x in a.log if p == cb3['player_position']]
said = [re.search(rb'teleported to (-?\d+) (-?\d+)', n) for n in notes(a, cb3)]
said = [m for m in said if m]
far = jumps and said and abs(teleport_spot(jumps[-1])[0] - int(said[-1].group(1))) < 1.5 and abs(teleport_spot(jumps[-1])[2] - int(said[-1].group(2))) < 1.5
check('random teleport drops you somewhere in the smp', any(b'teleported to' in n for n in notes(a, cb3)) and far, notes(a, cb3))
clear(a)
a.send(sb3['chat_command'], mstr('rtp')); time.sleep(0.4)
check('/rtp straight away is on cooldown', any(b'before teleporting again' in n for n in notes(a, cb3)), notes(a, cb3))
a.send(sb3['chat_command'], mstr('spawn')); time.sleep(1.0)

b = Java('HubTwo', 776); b.join(); time.sleep(0.8)
clear(a, b)
a.send(sb3['set_carried_item'], struct.pack('>H', 4)); use(a, sb3)
social = contents(a, cb3, 9)
names = [s and s[2] for s in social[-1][1][:45]] if social else []
spot = next((i for i, n in enumerate(names) if n and 'HubTwo' in n), None)
check('book opens the friends menu listing online players', screens(a, cb3) and rvarint(screens(a, cb3)[-1], 0)[0] == 9 and spot is not None, names[:6])
click(a, sb3, 9, spot if spot is not None else 0)
check('clicking a player sends a request', any(b'HubOne wants to be your friend' in n for n in notes(b, cb2)) and any(b'request sent to HubTwo' in n for n in notes(a, cb3)), notes(b, cb2))
clear(a, b)
b.send(sb2['chat_command'], mstr('friends')); time.sleep(0.4)
social2 = contents(b, cb2, 9)
first = social2[-1][1][0] if social2 else None
check('/friends shows the request first', first and 'HubOne' in first[2], first)
click(b, sb2, 9, 0)
check('clicking the request makes them friends', any(b'now friends' in n for n in notes(b, cb2)) and any(b'now friends' in n for n in notes(a, cb3)))
saved = open(os.path.join(BIN, 'players', 'hubone.txt')).read()
check('friends are saved in the profile', 'friend HubTwo' in saved, saved)
b.close(); time.sleep(0.4)
clear(a)
b = Java('HubTwo', 776); b.join(); time.sleep(0.8)
check('a friend joining is announced', any(b'your friend HubTwo joined' in n for n in notes(a, cb3)), notes(a, cb3))
clear(a, b)
b.send(sb2['chat_command'], mstr('friends')); time.sleep(0.4)
social2 = contents(b, cb2, 9)
spot = next((i for i, s in enumerate(social2[-1][1][:45]) if s and 'HubOne' in s[2]), None) if social2 else None
click(b, sb2, 9, spot if spot is not None else 0, 1)
check('right click removes a friend on both sides', 'friend' not in open(os.path.join(BIN, 'players', 'hubone.txt')).read() and any(b'no longer your friend' in n for n in notes(b, cb2)))

c = Java('HubOld', 47); c.join(); time.sleep(0.8)
with c.lock: win = [x for p, x in c.log if p == 0x30 and x[0] == 0]
check('1.8 gets the hub hotbar', win and b'Games' in win[-1] and b'Friends' in win[-1] and b'Info' in win[-1], len(win))
with c.lock: c.log.clear()
c.send(0x09, struct.pack('>H', 0))
c.send(0x08, struct.pack('>Q', 0xFFFFFFFFFFFFFFFF) + b'\xff' + struct.pack('>h', -1) + b'\x08\x08\x08'); time.sleep(0.4)
with c.lock: opened = [x for p, x in c.log if p == 0x2D]
check('1.8 right click compass opens the games menu', opened and opened[-1][0] == 7, opened)

d = Bedrock('HubRock'); d.join(); time.sleep(1.5)
def bcommand(text):
    raw = ('/' + text).encode()
    d.client.game([bp.uvarint(77) + bp.uvarint(len(raw)) + raw + b'\x00' + bytes(16) + b'\x00\x00\x00\x00']); time.sleep(0.6)
def forms(wanted):
    out = []
    for body in d.packets(100):
        form_id, p = rvarint(body, 0); n, p = rvarint(body, p)
        if form_id == wanted: out.append(json.loads(body[p:p + n]))
    return out
bcommand('games'); bcommand('info'); bcommand('friends')
check('bedrock /games, /info and /friends open forms', forms(1) and forms(2) and forms(1002), (len(forms(1)), len(forms(2)), len(forms(1002))))
check('bedrock friends form lists java players', forms(1002) and any('HubOne' in b['text'] for b in forms(1002)[-1]['buttons']), forms(1002) and forms(1002)[-1])
for x in (a, b, c, d): x.close()
print('failures:', fails)
