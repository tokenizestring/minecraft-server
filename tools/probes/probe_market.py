import time, sys, os, struct
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

def pos(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))

def hashed(item=None, count=0):
    return b'\x01' + varint(item) + varint(count) + varint(0) + varint(0) if item is not None else b'\x00'

def text_of(body, p):
    assert body[p] == 0x0A; p += 1
    while body[p]:
        tag = body[p]; n = struct.unpack('>H', body[p + 1:p + 3])[0]; p += 3 + n
        p += 1 if tag == 1 else 2 + struct.unpack('>H', body[p:p + 2])[0]
    return p + 1

def slots_of(body):
    window, p = rvarint(body, 0); state, p = rvarint(body, p); n, p = rvarint(body, p); out = []
    for _ in range(n + 1):
        count, p = rvarint(body, p)
        if count:
            item, p = rvarint(body, p); added, p = rvarint(body, p); removed, p = rvarint(body, p)
            for _ in range(added):
                kind, p = rvarint(body, p)
                lines, p = rvarint(body, p) if kind == 11 else (1, p)
                for _ in range(lines): p = text_of(body, p)
            out.append((item, count))
        else:
            out.append(None)
    return window, out[:-1], out[-1]

def contents(client, cb, window):
    with client.lock:
        return [slots_of(b) for p, b in client.log if p == cb['container_set_content'] and rvarint(b, 0)[0] == window]

def notes(client, cb):
    with client.lock:
        return [b for p, b in client.log if p == cb['system_chat']]

def say(client, sb, text, wait=0.4):
    client.send(sb['chat_command'], mstr(text)); time.sleep(wait)

for name in ('marketseller', 'marketbuyer'):
    path = os.path.join(BIN, 'players', name + '.txt')
    if os.path.exists(path): os.remove(path)

s = Java('MarketSeller', 777); s.join(); time.sleep(0.8)
first = contents(s, cb3, 0)
check('joining sends the player inventory', first and len(first[-1][1]) == 46, len(first[-1][1]) if first else None)
say(s, sb3, 'smp', 2.0)
with s.lock: spot = [x for p, x in s.log if p == cb3['player_position']][-1]
sx, sy, sz = int(struct.unpack('>d', spot[1:9])[0] // 1), int(struct.unpack('>d', spot[9:17])[0]), int(struct.unpack('>d', spot[17:25])[0] // 1)
import random
sx += random.randint(-40, 40); sz += random.randint(-40, 40)
with s.lock: s.log.clear()
s.send(sb3['set_carried_item'], struct.pack('>H', 0))
after = []
for depth in range(1, 20):
    s.move(sx + 0.5, sy - depth + 1, sz + 0.5, 0.0)
    s.send(sb3['player_action'], varint(0) + pos(sx, sy - depth, sz) + b'\x01' + varint(depth * 2))
    s.send(sb3['player_action'], varint(3) + pos(sx, sy - depth, sz) + b'\x01' + varint(depth * 2 + 1)); time.sleep(1.0)
    after = contents(s, cb3, 0)
    if after: break
mined = after[-1][1][36] if after else None
check('survival mining puts the drop in the hotbar', mined is not None and mined[1] == 1, (mined and items3[mined[0]], len(after)))

with s.lock: s.log.clear()
say(s, sb3, 'sell 5')
check('/sell lists the held item', any(b'listed 1' in n for n in notes(s, cb3)), notes(s, cb3))
with s.lock: emptied = [b for p, b in s.log if p == cb3['set_player_inventory']]
check('and empties the hand', emptied and emptied[-1] == b'\x00\x00', emptied)
say(s, sb3, 'sell 5')
check('selling with an empty hand is refused', any(b'hold the item' in n for n in notes(s, cb3)), None)

b = Java('MarketBuyer', 776); b.join(); time.sleep(0.8)
b.send(sb2['chat_command'], mstr('smp')); time.sleep(1.5)
with b.lock: b.log.clear()
say(b, sb2, 'buy', 0.6)
with b.lock: screens = [x for p, x in b.log if p == cb2['open_screen']]
menu = contents(b, cb2, 8)
check('/buy opens the market chest', screens and rvarint(screens[-1], 0)[0] == 8 and menu and len(menu[-1][1]) == 54, len(screens))
newest = max([i for i, x in enumerate(menu[-1][1][:45]) if x], default=0) if menu else 0
listing = menu[-1][1][newest] if menu else None
check('the listing shows the mined item for 26.2', listing and items2[listing[0]] == items3[mined[0]], listing and items2[listing[0]])
info = menu[-1][1][49] if menu else None
check('balance button is an emerald', info and items2[info[0]] == 'minecraft:emerald', info)

with b.lock: b.log.clear()
b.send(sb2['container_click'], varint(8) + varint(0) + struct.pack('>hb', newest, 0) + varint(0) + varint(0) + hashed()); time.sleep(0.5)
check('clicking the listing buys it', any(b'bought 1' in n for n in notes(b, cb2)), notes(b, cb2))
check('the seller is told', any(b'bought your' in n for n in notes(s, cb3)), None)
inv = contents(b, cb2, 0)
check('the item lands in the buyer hotbar', inv and inv[-1][1][36] and items2[inv[-1][1][36][0]] == items3[mined[0]], inv and inv[-1][1][36])
with b.lock: b.log.clear()
with s.lock: s.log.clear()
say(b, sb2, 'bal'); say(s, sb3, 'bal')
check('buyer paid 5', any(b'balance 95' in n for n in notes(b, cb2)), notes(b, cb2))
check('seller got 5', any(b'balance 105' in n for n in notes(s, cb3)), notes(s, cb3))

item2 = inv[-1][1][36][0]
with b.lock: b.log.clear()
b.send(sb2['container_click'], varint(0) + varint(1) + struct.pack('>hb', 36, 0) + varint(0) + varint(1) + struct.pack('>h', 36) + hashed() + hashed(item2, 1)); time.sleep(0.3)
b.send(sb2['container_click'], varint(0) + varint(1) + struct.pack('>hb', 20, 0) + varint(0) + varint(1) + struct.pack('>h', 20) + hashed(item2, 1) + hashed()); time.sleep(0.3)
check('honest inventory clicks need no resync', not contents(b, cb2, 0), len(contents(b, cb2, 0)))
b.send(sb2['container_click'], varint(0) + varint(1) + struct.pack('>hb', 21, 0) + varint(0) + varint(1) + struct.pack('>h', 21) + hashed(items2.index('minecraft:diamond'), 64) + hashed()); time.sleep(0.3)
fixed = contents(b, cb2, 0)
check('a faked diamond stack is undone', fixed and fixed[-1][1][21] is None and fixed[-1][1][20] and fixed[-1][1][20][0] == item2, fixed and fixed[-1][1][20:22])

with s.lock: s.log.clear()
say(s, sb3, 'pay MarketBuyer 10')
check('/pay moves money', any(b'paid MarketBuyer 10' in n for n in notes(s, cb3)) and any(b'paid you 10' in n for n in notes(b, cb2)), notes(s, cb3))

b.close(); s.close(); time.sleep(0.6)
saved = open(os.path.join(BIN, 'players', 'marketbuyer.txt')).read()
check('profiles are saved on leave', 'balance 105' in saved and 'slot 20 ' + items3[mined[0]] in saved, saved)

again = Java('MarketBuyer', 777); again.join(); time.sleep(0.8)
again.send(sb3['chat_command'], mstr('smp')); time.sleep(1.5)
back = contents(again, cb3, 0)
check('inventory comes back on rejoin', back and back[-1][1][20] and items3[back[-1][1][20][0]] == items3[mined[0]], back and back[-1][1][20])
again.close()

old = Java('Market18', 47); old.join(); time.sleep(0.8)
with old.lock: old.log.clear()
old.send(0x01, mstr('/buy')); time.sleep(0.6)
with old.lock: log18 = list(old.log)
opened = [b for p, b in log18 if p == 0x2D]
filled = [b for p, b in log18 if p == 0x30 and b[0] == 8]
check('1.8 /buy opens the market chest', opened and opened[-1][0] == 8 and filled and struct.unpack('>H', filled[-1][1:3])[0] == 54, (len(opened), len(filled)))
old.close()
print('failures:', fails)
