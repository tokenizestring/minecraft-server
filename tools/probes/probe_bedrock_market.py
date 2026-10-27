import time, sys, os, struct, json
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

BIN = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'build', 'bin')

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

cb3, sb3 = packet_ids('26_3')

def pos(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))

def notes(client):
    with client.lock:
        return [b for p, b in client.log if p == cb3['system_chat']]

def command(client, text):
    raw = ('/' + text).encode()
    client.client.game([bp.uvarint(77) + bp.uvarint(len(raw)) + raw + b'\x00' + bytes(16) + b'\x00\x00\x00\x00']); time.sleep(0.6)

def rstr(b, p):
    n, p = rvarint(b, p); return b[p:p + n], p + n

def tips(client):
    return [b for b in client.packets(9) if b[:3] == b'\x00\x00\x05']

def last_form(client, wanted):
    found = None
    for body in client.packets(100):
        form_id, p = rvarint(body, 0); text, p = rstr(body, p)
        found = json.loads(text) if form_id == wanted else found
    return found

def answer(client, form_id, data):
    client.client.game([bp.uvarint(101) + bp.uvarint(form_id) + b'\x01' + bp.uvarint(len(data)) + data.encode() + b'\x00']); time.sleep(0.6)

for name in ('bedrockseller', 'bedrockbuyer'):
    path = os.path.join(BIN, 'players', name + '.txt')
    if os.path.exists(path): os.remove(path)

s = Java('BedrockSeller', 777); s.join(); time.sleep(0.8)
s.send(sb3['chat_command'], mstr('smp')); time.sleep(2.0)
with s.lock: spot = [x for p, x in s.log if p == cb3['player_position']][-1]
sx, sy, sz = int(struct.unpack('>d', spot[1:9])[0] // 1), int(struct.unpack('>d', spot[9:17])[0]), int(struct.unpack('>d', spot[17:25])[0] // 1)
import random
sx += random.randint(-40, 40); sz += random.randint(-40, 40)
s.send(sb3['set_carried_item'], struct.pack('>H', 0))
for depth in range(1, 20):
    with s.lock: s.log.clear()
    s.move(sx + 1.5, sy - depth + 1, sz + 1.5, 0.0)
    s.send(sb3['player_action'], varint(0) + pos(sx + 1, sy - depth, sz + 1) + b'\x01' + varint(depth * 2))
    s.send(sb3['player_action'], varint(2) + pos(sx + 1, sy - depth, sz + 1) + b'\x01' + varint(depth * 2 + 1)); time.sleep(1.0)
    with s.lock: got = [x for p, x in s.log if p == cb3['container_set_content']]
    if got: break
with s.lock: s.log.clear()
s.send(sb3['chat_command'], mstr('sell 7')); time.sleep(0.5)
check('java seller lists a mined block for 7', any(b'listed ' in n for n in notes(s)), notes(s))

d = Bedrock('BedrockBuyer'); d.join(); time.sleep(1.5)
command(d, 'bal')
check('bedrock /bal shows the start balance', any(b'balance 100' in t for t in tips(d)), tips(d))
command(d, 'buy')
form = last_form(d, 1000)
mine = [i for i, b in enumerate(form['buttons']) if 'BedrockSeller' in b['text'] and ' - 7\n' in b['text']] if form else []
check('bedrock /buy opens a form with the java listing', form and form['type'] == 'form' and mine, form)
with s.lock: s.log.clear()
answer(d, 1000, str(mine[0]) if mine else '0')
check('tapping it buys the item', any(b'bought ' in t for t in tips(d)), tips(d)[-2:])
check('the java seller is told', any(b'BedrockBuyer bought your' in n for n in notes(s)), notes(s))
command(d, 'bal')
check('bedrock paid 7', any(b'balance 93' in t for t in tips(d)), tips(d)[-1:])

command(d, 'sell')
sell = last_form(d, 1001)
check('bedrock /sell opens a custom form with the bought item', sell and sell['type'] == 'custom_form' and len(sell['content'][0]['options']) == 1, sell)
answer(d, 1001, '[0,"9"]')
check('selling it from the form lists it', any(b'listed ' in t for t in tips(d)), tips(d)[-1:])
command(d, 'sell')
check('nothing left to sell is explained', any(b'nothing to sell' in t for t in tips(d)), tips(d)[-1:])

command(d, 'buy')
form = last_form(d, 1000)
own = [i for i, b in enumerate(form['buttons']) if 'yours' in b['text']] if form else []
check('own listing is marked as yours', len(own) == 1, form)
answer(d, 1000, str(own[0]) if own else '0')
check('tapping your own takes it back', any(b'took back' in t for t in tips(d)), tips(d)[-1:])

with s.lock: s.log.clear()
command(d, 'pay BedrockSeller 3')
check('bedrock /pay reaches the java player', any(b'paid BedrockSeller 3' in t for t in tips(d)) and any(b'BedrockBuyer paid you 3' in n for n in notes(s)), notes(s))
answer(d, 1000, '5')
check('a stale form answer does nothing harmful', not any(b'bought' in t for t in tips(d)[-1:]), tips(d)[-1:])

d.close(); s.close(); time.sleep(1.0)
saved = open(os.path.join(BIN, 'players', 'bedrockbuyer.txt')).read()
check('bedrock profile is saved on leave', 'balance 90' in saved and 'slot 0 ' in saved, saved)
print('failures:', fails)
