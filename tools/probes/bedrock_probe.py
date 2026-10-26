import socket, struct, time, zlib, base64, json, os, random

HOST, PORT = '127.0.0.1', 19132
import sys
if len(sys.argv) > 1 and sys.argv[1].isdigit(): PORT = int(sys.argv[1])
MAGIC = bytes.fromhex('00ffff00fefefefefdfdfdfd12345678')
PROTOCOL = 2193

def uvarint(v):
    out = bytearray()
    while True:
        b = v & 0x7F
        v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b)
            return bytes(out)

def ruvarint(b, p):
    r = s = 0
    while True:
        x = b[p]; p += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            return r, p

def address(host, port):
    return b'\x04' + bytes(~int(x) & 0xFF for x in host.split('.')) + struct.pack('>H', port)

class Client:
    def __init__(self, mtu=1400):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.s.settimeout(3)
        self.mtu = mtu
        self.seq = 0
        self.reliable = 0
        self.order = 0
        self.split_id = 0
        self.guid = random.getrandbits(63)
        self.splits = {}
        self.compression = False
        self.seen_reliable = set()
        self.pending_order = {}
        self.next_order = 0

    def raw(self, data):
        self.s.sendto(data, (HOST, PORT))

    def recv_raw(self, timeout=3):
        self.s.settimeout(timeout)
        data, _ = self.s.recvfrom(4096)
        return data

    def send_frames(self, body):
        limit = self.mtu - 28 - 4 - 24
        parts = [body[i:i + limit] for i in range(0, len(body), limit)] if len(body) > limit else [body]
        order = self.order; self.order += 1
        sid = self.split_id; self.split_id += 1
        for index, part in enumerate(parts):
            split = len(parts) > 1
            frame = bytes([(3 << 5) | (0x10 if split else 0)]) + struct.pack('>H', len(part) * 8)
            frame += struct.pack('<I', self.reliable)[:3] + struct.pack('<I', order)[:3] + b'\x00'
            self.reliable += 1
            if split:
                frame += struct.pack('>IHI', len(parts), sid, index)
            frame += part
            self.raw(bytes([0x84]) + struct.pack('<I', self.seq)[:3] + frame)
            self.seq += 1

    def messages(self, timeout=3):
        deadline = time.time() + timeout
        out = []
        while time.time() < deadline:
            try:
                d = self.recv_raw(max(0.05, deadline - time.time()))
            except socket.timeout:
                break
            if d[0] & 0x40 or d[0] & 0x20:
                continue
            if not d[0] & 0x80:
                out.append(d); continue
            seq = int.from_bytes(d[1:4], 'little')
            self.raw(b'\xc0' + struct.pack('>H', 1) + b'\x01' + struct.pack('<I', seq)[:3])
            p = 4
            while p < len(d):
                flags = d[p]; p += 1
                length = (struct.unpack('>H', d[p:p + 2])[0] + 7) // 8; p += 2
                rel = flags >> 5
                reliable = order = None
                if rel in (2, 3, 4, 6, 7):
                    reliable = int.from_bytes(d[p:p + 3], 'little'); p += 3
                if rel in (1, 4): p += 3
                if rel in (1, 3, 4, 7):
                    order = int.from_bytes(d[p:p + 3], 'little'); p += 4
                split = None
                if flags & 0x10:
                    split = struct.unpack('>IHI', d[p:p + 10]); p += 10
                body = d[p:p + length]; p += length
                if reliable is not None:
                    if reliable in self.seen_reliable:
                        continue
                    self.seen_reliable.add(reliable)
                if split:
                    count, sid, idx = split
                    parts = self.splits.setdefault(sid, {})
                    parts[idx] = body
                    if len(parts) < count:
                        continue
                    body = b''.join(parts[i] for i in range(count)); del self.splits[sid]
                if order is None:
                    out.append(body); continue
                self.pending_order[order] = body
                while self.next_order in self.pending_order:
                    out.append(self.pending_order.pop(self.next_order)); self.next_order += 1
        return out

    def game(self, packets, compress=False):
        batch = b''.join(uvarint(len(p)) + p for p in packets)
        if self.compression:
            if compress:
                c = zlib.compressobj(6, zlib.DEFLATED, -15)
                batch = b'\x00' + c.compress(batch) + c.flush()
            else:
                batch = b'\xff' + batch
        self.send_frames(b'\xfe' + batch)

    def decode_game(self, msg):
        assert msg[0] == 0xFE
        body = msg[1:]
        if self.compression:
            assert body[0] == 0xFF, 'server compression byte %x' % body[0]
            body = body[1:]
        out, p = [], 0
        while p < len(body):
            n, p = ruvarint(body, p)
            pkt = body[p:p + n]; p += n
            header, q = ruvarint(pkt, 0)
            out.append((header & 0x3FF, pkt[q:]))
        return out

def ping():
    c = Client()
    c.raw(b'\x01' + struct.pack('>Q', 1234) + MAGIC + struct.pack('>Q', c.guid))
    d = c.recv_raw()
    assert d[0] == 0x1C and struct.unpack('>Q', d[1:9])[0] == 1234 and d[17:33] == MAGIC
    n = struct.unpack('>H', d[33:35])[0]
    return d[35:35 + n].decode()

def connect(c):
    c.raw(b'\x05' + MAGIC + b'\x0b' + b'\x00' * (c.mtu - 28 - 18))
    d = c.recv_raw(); assert d[0] == 0x06 and d[1:17] == MAGIC, d[:4]
    mtu1 = struct.unpack('>H', d[26:28])[0]
    c.raw(b'\x07' + MAGIC + address(HOST, PORT) + struct.pack('>H', mtu1) + struct.pack('>Q', c.guid))
    d = c.recv_raw(); assert d[0] == 0x08 and d[1:17] == MAGIC, d[:4]
    c.send_frames(b'\x09' + struct.pack('>QQ', c.guid, int(time.time() * 1000)) + b'\x00')
    msgs = c.messages(1)
    assert any(m[0] == 0x10 for m in msgs), [m[:1] for m in msgs]
    c.send_frames(b'\x13' + address(HOST, PORT) + address('0.0.0.0', 0) * 20 + struct.pack('>QQ', 1, 2))
    c.send_frames(b'\x00' + struct.pack('>Q', 777))
    msgs = c.messages(1)
    assert any(m[0] == 0x03 and struct.unpack('>Q', m[1:9])[0] == 777 for m in msgs), 'no connected pong'
    return mtu1

def jwt(payload):
    enc = lambda o: base64.urlsafe_b64encode(json.dumps(o).encode()).rstrip(b'=').decode()
    return enc({'alg': 'ES384', 'x5u': 'key'}) + '.' + enc(payload) + '.' + base64.urlsafe_b64encode(os.urandom(96)).rstrip(b'=').decode()

def login(name, protocol=PROTOCOL, skin_kb=40):
    c = Client()
    mtu = connect(c)
    c.game([uvarint(193) + struct.pack('>i', protocol)])
    games = [p for m in c.messages(1) if m[0] == 0xFE for p in c.decode_game(m)]
    if protocol != PROTOCOL:
        assert games and games[0][0] == 2, games
        return 'play status %d for protocol %d' % (struct.unpack('>i', games[0][1])[0], protocol)
    assert games and games[0][0] == 143, games
    threshold, algorithm = struct.unpack('<HH', games[0][1][:4])
    c.compression = True
    chain = json.dumps({'AuthenticationType': 2, 'Certificate': json.dumps({'chain': [jwt({'identityPublicKey': 'x'}), jwt({'extraData': {'displayName': name, 'XUID': '2535400000000001', 'identity': 'uuid'}})]}), 'Token': ''})
    skin = base64.b64encode(os.urandom(skin_kb * 512) + b'\x00' * (skin_kb * 512)).decode()
    client = jwt({'ThirdPartyName': name, 'SkinData': skin, 'DeviceOS': 7})
    request = struct.pack('<I', len(chain)) + chain.encode() + struct.pack('<I', len(client)) + client.encode()
    c.game([uvarint(1) + struct.pack('>i', PROTOCOL) + uvarint(len(request)) + request], compress=True)
    games = [p for m in c.messages(2) if m[0] == 0xFE for p in c.decode_game(m)]
    assert [g[0] for g in games] == [2, 6], [g[0] for g in games]
    assert struct.unpack('>i', games[0][1])[0] == 0
    reference = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'captures', 'capture_bedrock_bds', '003_6.bin')
    assert games[1][1] == open(reference, 'rb').read(), 'resource packs info differs from BDS: ' + games[1][1].hex()
    c.game([uvarint(8) + uvarint(2) + uvarint(19) + b'downloadingfinished'])
    games = [p for m in c.messages(1) if m[0] == 0xFE for p in c.decode_game(m)]
    assert [g[0] for g in games] == [7], [g[0] for g in games]
    assert games[0][1] == open(reference.replace('003_6', '004_7'), 'rb').read(), 'pack stack differs from BDS'
    c.game([uvarint(8) + uvarint(3) + uvarint(25) + b'resourcepackstackfinished'])
    games = [p for m in c.messages(2) if m[0] == 0xFE for p in c.decode_game(m)]
    ids = [g[0] for g in games]
    assert ids[:8] == [313, 337, 11, 162, 122, 70, 121, 45] and ids.count(58) == 25 and ids[-2:] == [2, 39], ids
    sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
    import bedrock_startgame
    parsed = bedrock_startgame.parse(games[2][1])
    assert parsed['exact'] and parsed['hashes'] and parsed['tail'] == '01 00 00 00 00 00 00 00', parsed['consumed']
    assert struct.unpack('>i', games[-2][1])[0] == 3
    for pid, body in games:
        if pid != 58: continue
        p = 0
        for _ in range(3): _, p = ruvarint(body, p)
        count, p = ruvarint(body, p); assert count == 0
        limit, p = ruvarint(body, p); assert limit == 1
        highest, p = ruvarint(body, p); assert highest == 24
        assert body[p] == 0 and body[p + 1] == 0; p += 2
        n, p = ruvarint(body, p); data = body[p:p + n]; assert p + n == len(body)
        assert data == bytes([1, 2]) * 24 + bytes([0]), data.hex()
    subchunk_request = uvarint(175) + uvarint(0) + uvarint(24) + b''.join(struct.pack('bbb', 0, dy, 0) for dy in range(-4, 20)) + struct.pack('<iii', 0, 0, 0)
    c.game([subchunk_request])
    reply = [g for m in c.messages(1) if m[0] == 0xFE for g in c.decode_game(m) if g[0] == 174]
    assert reply, 'no subchunk reply'
    b = reply[0][1]; q = 1; _, q = ruvarint(b, q); q += 12; entries, q = ruvarint(b, q)
    grass = 0
    for _ in range(entries):
        q += 3; result = b[q]; flag = b[q + 1]; q += 2
        if flag:
            size, q = ruvarint(b, q); section = b[q:q + size]; q += size
            words = struct.unpack('<128I', section[4:4 + 512]); grass += sum(bin(w).count('1') for w in words)
        q += 5
    assert q == len(b) and grass == 25, (q, len(b), grass)
    c.game([uvarint(113) + uvarint(1)])
    c.game([uvarint(144) + struct.pack('<5f', 0, 0, 8.5, -40.0, 8.5) + b'\x00' * 40])
    games = [p for m in c.messages(1) if m[0] == 0xFE for p in c.decode_game(m)]
    assert any(g[0] == 19 for g in games), [g[0] for g in games]
    return 'mtu %d, threshold %d, login %.1f KB, 25 chunks with a 5x5 grass platform, spawned, void rescue ok' % (mtu, threshold, len(request) / 1024)

if __name__ == '__main__':
    fails = 0
    def check(label, fn):
        global fails
        try:
            print('PASS', label, '->', fn())
        except Exception as e:
            fails += 1
            print('FAIL', label, '->', repr(e))

    check('unconnected ping', ping)
    check('login small', lambda: login('Steve', skin_kb=1))
    check('login 60kb split+deflate', lambda: login('BigSkinPlayer', skin_kb=60))
    check('old client', lambda: login('Old', protocol=2000))
    check('new client', lambda: login('New', protocol=3000))
    check('ping after', ping)
    print('failures:', fails)
