import socket, struct, time, json, sys, os

HOST, PORT = '127.0.0.1', 25565
PROTOCOLS = [4, 5, 47, 107, 108, 109, 110, 210, 315, 316, 335, 338, 340, 393, 401, 404, 477, 480, 485, 490, 498, 573, 575, 578, 735, 736, 751, 753, 754, 755, 756, 757, 758, 759, 760, 761, 762, 763, 764, 765, 766, 767, 768, 769, 770, 771, 772, 773, 774, 775, 776, 777]

def varint(v):
    v &= 0xFFFFFFFF
    out = bytearray()
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)

def rvarint(b, p):
    r = s = 0
    while True:
        x = b[p]; p += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            if r & 0x80000000: r -= 1 << 32
            return r, p

def mstr(t):
    t = t.encode()
    return varint(len(t)) + t

class Conn:
    def __init__(self):
        self.s = socket.create_connection((HOST, PORT), timeout=5)
        self.rx = b''

    def send(self, pid, body=b''):
        p = varint(pid) + body
        self.s.sendall(varint(len(p)) + p)

    def recv(self, timeout=5):
        deadline = time.time() + timeout
        while True:
            try:
                n, p = rvarint(self.rx, 0)
                if len(self.rx) >= p + n:
                    frame = self.rx[p:p + n]; self.rx = self.rx[p + n:]
                    pid, q = rvarint(frame, 0)
                    return pid, frame[q:]
            except IndexError:
                pass
            left = deadline - time.time()
            if left <= 0: return None
            self.s.settimeout(left)
            try:
                d = self.s.recv(65536)
            except socket.timeout:
                return None
            if not d: return None
            self.rx += d

def handshake(c, proto, nxt):
    c.send(0, varint(proto) + mstr('localhost') + struct.pack('>H', PORT) + varint(nxt))

def login_start(proto, name):
    body = mstr(name)
    if proto == 759: body += b'\x00'
    elif proto == 760: body += b'\x00\x00'
    elif 761 <= proto <= 763: body += b'\x00'
    elif proto >= 764: body += b'\x11' * 16
    return body

def status(proto):
    c = Conn(); handshake(c, proto, 1); c.send(0)
    pid, body = c.recv()
    n, p = rvarint(body, 0)
    js = json.loads(body[p:p + n])
    c.send(1, struct.pack('>q', 1234567))
    pid2, body2 = c.recv()
    assert pid2 == 1 and struct.unpack('>q', body2)[0] == 1234567, 'pong'
    return js['version']

def parse_login_success(proto, body):
    p = 0
    if proto < 735:
        n, p = rvarint(body, p); uuid = body[p:p + n].decode(); p += n
        if proto < 5: assert len(uuid) == 32
        else: assert len(uuid) == 36
    else:
        p += 16
    n, p = rvarint(body, p); name = body[p:p + n].decode(); p += n
    if proto >= 759:
        cnt, p = rvarint(body, p); assert cnt == 0
    if proto in (766, 767):
        assert body[p] == 1; p += 1
    if proto >= 776:
        p += 16
        ref = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'captures', 'capture_%s' % MODERN[proto]['tag'], 'login_success.bin')
        if os.path.exists(ref):
            vanilla = open(ref, 'rb').read()
            assert len(vanilla) - len('capture') == len(body) - len(name), 'login success shape differs from vanilla'
    assert p == len(body), 'trailing bytes in login success (%d of %d)' % (p, len(body))
    return name

MODERN = {
    776: dict(tag='26_2', features=0x0C, known=0x0E, tags=0x0D, login=0x31, center=0x5E, position=0x48, chunk=0x2D, event=0x26, byte_bitsets=False, optional_prev=False),
    777: dict(tag='26_3', features=0x0D, known=0x0F, tags=0x0E, login=0x32, center=0x60, position=0x49, chunk=0x2E, event=0x27, byte_bitsets=True, optional_prev=True),
}

def rstr(b, p):
    n, p = rvarint(b, p)
    return b[p:p + n].decode(), p + n

def decode_config(frames, ids):
    regs, tags = {}, {}
    for pid, body in frames:
        if pid == 7:
            reg, q = rstr(body, 0)
            n, q = rvarint(body, q)
            names = []
            for _ in range(n):
                name, q = rstr(body, q); assert body[q] == 0; q += 1; names.append(name)
            assert q == len(body)
            regs[reg] = names
        elif pid == ids['tags']:
            n, q = rvarint(body, 0)
            for _ in range(n):
                reg, q = rstr(body, q)
                tn, q = rvarint(body, q)
                for _ in range(tn):
                    name, q = rstr(body, q)
                    vn, q = rvarint(body, q)
                    values = []
                    for _ in range(vn):
                        v, q = rvarint(body, q); values.append(v)
                    tags.setdefault(reg, {})[name] = values
            assert q == len(body)
    named = {}
    for reg, items in tags.items():
        named[reg] = {name: frozenset(regs[reg][v] if reg in regs else v for v in values) for name, values in items.items()}
    return regs, named

def reference_frames(tag):
    path = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'captures', 'capture_' + tag, 'registry.bin')
    if not os.path.exists(path): return None
    blob = open(path, 'rb').read()
    frames, p = [], 0
    while p < len(blob):
        n, p = rvarint(blob, p)
        pid, q = rvarint(blob, p)
        frames.append((pid, blob[q:p + n])); p += n
    return frames

def parse_login_modern(body, ids):
    p = 4 + 1
    n, p = rvarint(body, p)
    for _ in range(n): _, p = rstr(body, p)
    for _ in range(3): _, p = rvarint(body, p)
    p += 3
    _, p = rvarint(body, p)
    dim, p = rstr(body, p)
    p += 8 + 1
    prev = body[p]; p += 1
    assert prev == (0 if ids['optional_prev'] else 0xFF), 'previous gamemode byte %d' % prev
    p += 2
    death = body[p]; p += 1; assert death == 0
    _, p = rvarint(body, p); _, p = rvarint(body, p)
    p += 2
    assert p == len(body), 'login length %d vs %d' % (p, len(body))
    return dim

def parse_chunk_modern(body, byte_bitsets):
    p = 8
    hm, p = rvarint(body, p)
    for _ in range(hm):
        t, p = rvarint(body, p); n, p = rvarint(body, p); p += 8 * n
    size, p = rvarint(body, p)
    end = p + size
    blocks = 0
    for s in range(24):
        bc = struct.unpack('>h', body[p:p + 2])[0]; p += 4
        blocks += bc
        for kind, volume in (('block', 4096), ('biome', 64)):
            bits = body[p]; p += 1
            if bits == 0:
                _, p = rvarint(body, p)
            else:
                pl, p = rvarint(body, p)
                for _ in range(pl): _, p = rvarint(body, p)
                per = 64 // bits
                p += 8 * ((volume + per - 1) // per)
    assert p == end, 'section size mismatch %d vs %d' % (p, end)
    be, p = rvarint(body, p); assert be == 0
    masks = []
    for _ in range(4):
        n, p = rvarint(body, p)
        width = 1 if byte_bitsets else 8
        masks.append(int.from_bytes(body[p:p + width * n], 'little' if byte_bitsets else 'big')); p += width * n
    counts = []
    for _ in range(2):
        n, p = rvarint(body, p); counts.append(n)
        for _ in range(n):
            ln, p = rvarint(body, p); assert ln == 2048; p += ln
    assert p == len(body), 'chunk trailing %d vs %d' % (p, len(body))
    assert bin(masks[0]).count('1') == counts[0], 'sky mask %x vs %d arrays' % (masks[0], counts[0])
    return blocks

def play_modern(proto, name):
    ids = MODERN[proto]
    c = Conn(); handshake(c, proto, 2); c.send(0, login_start(proto, name))
    pid, body = c.recv(); assert pid == 2, 'expected login success got %r' % pid
    parse_login_success(proto, body)
    c.send(3)
    config = []
    while True:
        r = c.recv(); assert r, 'config stalled after %r' % [x[0] for x in config]
        config.append(r)
        if r[0] == 3: break
    regs, tags = decode_config(config, ids)
    assert any(pid == ids['known'] and ids['tag'].replace('_', '.').encode() in b for pid, b in config), 'known pack version'
    note = 'no reference'
    ref = reference_frames(ids['tag'])
    if ref:
        vregs, vtags = decode_config(ref, ids)
        assert list(regs) == list(vregs), 'registry list differs from vanilla'
        for reg in vregs: assert set(regs[reg]) == set(vregs[reg]), 'entries differ ' + reg
        assert set(tags) == set(vtags), 'tag registries differ'
        for reg in vtags: assert tags[reg] == vtags[reg], 'tags differ ' + reg
        note = 'matches vanilla: %d registries, %d tags' % (len(regs), sum(len(t) for t in tags.values()))
    c.send(3)
    seen = {}
    blocks = 0
    deadline = time.time() + 5
    while time.time() < deadline:
        r = c.recv(1)
        if not r: break
        seen[r[0]] = seen.get(r[0], 0) + 1
        if r[0] == ids['login']: assert parse_login_modern(r[1], ids) == 'minecraft:overworld'
        if r[0] == ids['event']: break
    assert seen.get(ids['login']) == 1 and seen.get(ids['center']) == 1 and seen.get(ids['position']) == 1 and seen.get(ids['chunk']) == 169, seen
    c.send(0x00, varint(1))
    c.send(0x1E, struct.pack('>dddB', 8.5, -40.0, 8.5, 0))
    r = c.recv()
    while r and r[0] != ids['position']: r = c.recv()
    assert r, 'no rescue teleport'
    c.s.close()
    return '%s, play %s' % (note, {hex(k): v for k, v in seen.items()})

def play_1_8(name):
    c = Conn(); handshake(c, 47, 2); c.send(0, login_start(47, name))
    pid, body = c.recv(); assert pid == 2
    parse_login_success(47, body)
    order = []
    while len(order) < 4:
        r = c.recv(); assert r, order
        if r[0] in (0x38, 0x0C): continue
        order.append(r[0])
        if r[0] == 0x21:
            x, z, gu, mask = struct.unpack('>ii?H', r[1][:11])
            n, p = rvarint(r[1], 11)
            assert n == 12544 and len(r[1]) == p + n and mask == 1 << (59 >> 4), (n, len(r[1]), mask)
            assert r[1][p + 2940 * 2] == 32
    assert order == [0x01, 0x05, 0x21, 0x08], order
    c.send(0x04, struct.pack('>ddd?', 8.5, -40.0, 8.5, False))
    r = c.recv()
    while r and r[0] != 0x08: r = c.recv()
    assert r, 'no rescue teleport'
    c.s.close()
    return 'play order %s' % [hex(x) for x in order]

def kicked(proto, name):
    c = Conn(); handshake(c, proto, 2); c.send(0, login_start(proto, name))
    pid, body = c.recv()
    assert pid == 0, 'expected login disconnect got %r' % pid
    n, p = rvarint(body, 0)
    return json.loads(body[p:p + n])['text']

def legacy():
    s = socket.create_connection((HOST, PORT), timeout=5)
    s.sendall(b'\xfe\x01\xfa')
    d = b''
    while True:
        x = s.recv(4096)
        if not x: break
        d += x
    assert d[0] == 0xFF
    n = struct.unpack('>H', d[1:3])[0]
    return d[3:3 + n * 2].decode('utf-16-be').split('\x00')

def garbage():
    out = []
    for payload in (b'\xff\xff\xff\xff\xff\xff', b'\x05\x00', b'\x00', b'\x10' + b'\x00' * 3):
        s = socket.create_connection((HOST, PORT), timeout=3)
        s.sendall(payload)
        try:
            s.settimeout(3); out.append(s.recv(10) == b'')
        except socket.timeout:
            out.append('open')
        s.close()
    return out

fails = 0
def check(label, fn):
    global fails
    try:
        print('PASS', label, '->', fn())
    except Exception as e:
        fails += 1
        print('FAIL', label, '->', repr(e))

for proto in PROTOCOLS:
    check('status %d' % proto, lambda: status(proto))
check('legacy ping', legacy)
for proto in PROTOCOLS:
    if proto in (47, 776, 777): continue
    check('login %d' % proto, lambda: kicked(proto, 'probe%d' % proto))
check('play 26.2', lambda: play_modern(776, 'probe_262'))
check('play 26.3', lambda: play_modern(777, 'probe_263'))
check('play 1.8', lambda: play_1_8('probe_18'))
check('garbage', garbage)
check('status after garbage', lambda: status(776))
print('failures:', fails)
