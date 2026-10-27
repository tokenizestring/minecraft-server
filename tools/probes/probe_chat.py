import time, sys, os, json, struct, re
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

TRICKY = 'héllo "quoted" back\\slash 😀 §cred'
LONG = 'this is a much longer message that has to wrap over several sidebar lines nicely'
LINES, WRAP, SECTION = 10, 28, '§'

def ids(client):
    return packet_ids('26_3' if client.protocol == 777 else '26_2')

def java_say(client, text):
    if client.protocol == 47:
        client.send(0x01, mstr(text))
    else:
        cb, sb = ids(client)
        client.send(sb['chat'], mstr(text) + struct.pack('>qq', int(time.time() * 1000), 0) + b'\x00' + varint(0) + bytes(3) + b'\x00')

def java_command(client, text):
    if client.protocol == 47:
        client.send(0x01, mstr('/' + text))
    else:
        cb, sb = ids(client)
        client.send(sb['chat_command'], mstr(text))

def bedrock_say(client, text):
    body = b'\x00' + bp.uvarint(1) + b'\x01' + bp.uvarint(len(client.name)) + client.name.encode() + bp.uvarint(len(text.encode())) + text.encode() + b'\x00\x00\x00'
    client.client.game([bp.uvarint(9) + body])

def bedrock_command(client, text):
    command = ('/' + text).encode()
    body = bp.uvarint(len(command)) + command + b'\x00' + bytes(16) + b'\x00' + b'\x00' + b'\x00' + b'\x00'
    client.client.game([bp.uvarint(77) + body])

def modified_utf8(raw):
    return raw.decode('utf-8', 'surrogatepass').encode('utf-16', 'surrogatepass').decode('utf-16')

def rstr(b, p):
    n, p = rvarint(b, p); return b[p:p + n].decode(), p + n

def nbt_text(b, p):
    assert b[p] == 0x08; n = struct.unpack('>H', b[p + 1:p + 3])[0]
    return modified_utf8(b[p + 3:p + 3 + n]), p + 3 + n

def snapshot(client):
    with client.lock:
        return list(client.log)

def java_sidebar(client):
    log = snapshot(client)
    if client.protocol == 47:
        assert all(body[-1] == 2 for pid, body in log if pid == 0x02), 'only action-bar notices allowed'
        scores = {}
        for pid, body in log:
            if pid == 0x3B:
                name, p = rstr(body, 0); mode = body[p]
                scores = {}
                if mode == 0:
                    title, p = rstr(body, p + 1); kind, p = rstr(body, p)
                    assert (name, title, kind, p) == ('chat', 'Chat', 'integer', len(body))
            elif pid == 0x3C:
                owner, p = rstr(body, 0); action = body[p]; objective, p = rstr(body, p + 1); value, p = rvarint(body, p)
                assert action == 0 and objective == 'chat' and p == len(body) and len(owner.encode('utf-16-le')) // 2 <= 40
                scores[owner[:-2]] = value
        return [k for k, v in sorted(scores.items(), key=lambda kv: -kv[1])]
    cb, sb = ids(client)
    for pid, body in log:
        if pid == cb['system_chat']:
            text, q = nbt_text(body, 0); assert body[q:] == b'\x01', 'only action-bar notices allowed'
    creates = [body for pid, body in log if pid == cb['set_objective']]
    assert len(creates) == 1, len(creates)
    name, p = rstr(creates[0], 0); assert name == 'chat' and creates[0][p] == 0
    title, p = nbt_text(creates[0], p + 1); assert title == 'Chat' and creates[0][p:] == b'\x00\x01\x00', creates[0][p:]
    displays = [body for pid, body in log if pid == cb['set_display_objective']]
    assert all(x in (b'\x01\x04chat', b'\x01\x00') for x in displays), displays
    if not displays or displays[-1] == b'\x01\x00':
        return []
    scores = {}
    for pid, body in log:
        if pid == cb['set_score']:
            owner, p = rstr(body, 0); objective, p = rstr(body, p); value, p = rvarint(body, p)
            assert body[p] == 1; text, p = nbt_text(body, p + 1)
            assert objective == 'chat' and body[p:] == b'\x00', body[p:]
            scores[owner] = (value, text)
        elif pid == cb['reset_score']:
            owner, p = rstr(body, 0); assert body[p] == 1; objective, p = rstr(body, p + 1)
            assert objective == 'chat' and p == len(body)
            scores.pop(owner, None)
    return [t for v, t in sorted(scores.values(), key=lambda x: -x[0])]

def bedrock_sidebar(client):
    log = snapshot(client)
    assert all(body[:3] == b'\x00\x00\x05' for pid, body in log if pid == 9), 'only tip notices allowed'
    order = [pid for pid, _ in log if pid in (106, 108)]
    if not order or order[-1] == 106:
        return []
    for pid, body in log:
        if pid == 107:
            p = 0; fields = []
            for _ in range(4):
                s, p = rstr(body, p); fields.append(s)
            assert fields == ['sidebar', 'chat', 'Chat', 'dummy'] and body[p:] == b'\x02', (fields, body[p:])
    body = [body for pid, body in log if pid == 108][-1]
    count, p = rvarint(body, 0); rows = []
    for _ in range(count):
        variant, p = rvarint(body, p); action, p = rstr(body, p); ident, p = rvarint(body, p); objective, p = rstr(body, p)
        value = struct.unpack('<i', body[p:p + 4])[0]; text, p = rstr(body, p + 4)
        assert variant == 3 and action == 'changefakeplayer' and objective == 'chat'
        rows.append((value, text))
    assert p == len(body)
    return [t for v, t in sorted(rows, key=lambda x: -x[0])]

def notices(client):
    log = snapshot(client)
    if isinstance(client, Bedrock):
        return [rstr(body, 3)[0] for pid, body in log if pid == 9]
    if client.protocol == 47:
        return [json.loads(rstr(body, 0)[0])['text'] for pid, body in log if pid == 0x02]
    cb, sb = ids(client)
    return [nbt_text(body, 0)[0] for pid, body in log if pid == cb['system_chat']]

def wrap(message):
    out, line = [], ''
    for word in message.split(' '):
        while len(word) > WRAP:
            if line: out.append(line); line = ''
            out.append(word[:WRAP]); word = word[WRAP:]
        if len(line) + (1 if line else 0) + len(word) > WRAP and line:
            out.append(line); line = word
        else:
            line = (line + ' ' + word) if line else word
    if line: out.append(line)
    return out

def cut_1_8(lines):
    out = []
    for l in lines:
        units, cut = 0, ''
        for ch in l:
            w = 2 if ord(ch) > 0xFFFF else 1
            if units + w > 38: break
            units += w; cut += ch
        out.append(cut)
    return out

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

def compare(label, client, sidebar, want):
    try:
        lines = sidebar(client)
        want = cut_1_8(want) if getattr(client, 'protocol', 0) == 47 else want
        check(label, lines == want, '' if lines == want else (lines, want))
    except AssertionError as e:
        check(label + ' (packet layout)', False, e)

a = Java('Chat263', 777); a.join(); time.sleep(0.4)
b = Java('Chat262', 776); b.join(); time.sleep(0.4)
c = Java('Chat18', 47); c.join(); time.sleep(0.4)
d = Bedrock('ChatBedrock'); d.join(); time.sleep(1.5)

def command_tree(client):
    cb, sb = ids(client)
    body = [body for pid, body in snapshot(client) if pid == cb['commands']][0]
    count, p = rvarint(body, 0); nodes = []
    for _ in range(count):
        flags = body[p]; p += 1
        n, p = rvarint(body, p); children = []
        for _ in range(n):
            child, p = rvarint(body, p); children.append(child)
        name = ''
        if flags & 3 == 1:
            name, p = rstr(body, p)
        if flags & 3 == 2:
            name, p = rstr(body, p); parser, p = rvarint(body, p)
            if parser == 5: _, p = rvarint(body, p)
        nodes.append((flags, children, name))
    root, p = rvarint(body, p)
    assert p == len(body), 'trailing bytes in commands packet'
    chat = nodes[nodes[root][1][0]]
    return chat[2], chat[0], sorted(nodes[c][2] for c in chat[1])

for label, client in (('26.3', a), ('26.2', b)):
    try:
        check('%s gets /chat command tree' % label, command_tree(client) == ('chat', 5, ['0', '1', 'off', 'on']), command_tree(client))
    except Exception as e:
        check('%s gets /chat command tree' % label, False, e)

said = []
for client, text in ((a, 'hi from 26.3'), (b, 'hi from 26.2'), (c, 'hi from 1.8')):
    java_say(client, text); said.append((client.name, text)); time.sleep(0.25)
for text in ('hi from bedrock', TRICKY, LONG):
    bedrock_say(d, text); said.append(('ChatBedrock', text)); time.sleep(0.25)
time.sleep(0.8)

colors = {}
score_id = ids(a)[0]['set_score']
seen = []
for pid, body in snapshot(a):
    if pid == score_id:
        owner, p = rstr(body, 0); objective, p = rstr(body, p); value, p = rvarint(body, p)
        seen.append(nbt_text(body, p + 1)[0])
for line in seen:
    m = re.match('<' + SECTION + '(.)([^' + SECTION + ']+)' + SECTION + 'r> ', line)
    if m: colors[m.group(2)] = m.group(1)
order = ['Chat263', 'Chat262', 'Chat18', 'ChatBedrock']
check('every speaker has a colour, neighbours differ', all(x in colors for x in order) and all(colors[x] != colors[y] for x, y in zip(order, order[1:])), colors)

def line_of(name, text):
    return '<%s%s%s%sr> %s' % (SECTION, colors.get(name, '?'), name, SECTION, text)

expected = [l for name, text in said for l in wrap(line_of(name, text))][-LINES:]
clients = (('26.3', a, java_sidebar), ('26.2', b, java_sidebar), ('1.8', c, java_sidebar), ('bedrock', d, bedrock_sidebar))
for label, client, sidebar in clients:
    compare('%s sidebar shows last %d coloured lines, no normal chat' % (label, LINES), client, sidebar, expected)

java_command(a, 'chat off'); java_command(c, 'chat 0'); bedrock_command(d, 'chat off'); time.sleep(0.6)
java_say(b, 'while hidden'); time.sleep(0.6)
after = (expected + wrap(line_of('Chat262', 'while hidden')))[-LINES:]
for label, client, sidebar in clients:
    compare('%s after /chat off' % label if client is not b else '26.2 still sees chat', client, sidebar, after if client is b else [])
for label, client in (('26.3', a), ('1.8', c), ('bedrock', d)):
    check('%s got "chat off" notice' % label, notices(client)[-1:] == ['chat off'], notices(client))

java_command(a, 'chat 1'); bedrock_command(d, 'chat on'); java_command(c, 'nonsense'); time.sleep(0.6)
compare('26.3 back after /chat 1', a, java_sidebar, after)
compare('bedrock back after /chat on', d, bedrock_sidebar, after)
check('unknown command notice', notices(c)[-1:] == ['unknown command'], notices(c))

print('waiting for fade...')
for _ in range(32):
    d.move(8.5, 65.0, 8.5, 0.0); time.sleep(0.5)
for label, client, sidebar in clients:
    compare('%s faded to empty' % label, client, sidebar, [])

for x in (a, b, c): x.close()
d.close()
print('failures:', fails)
