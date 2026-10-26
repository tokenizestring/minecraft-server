import socket, select, struct, time, zlib, hashlib, json, base64, os, sys, glob, traceback

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [os.path.join(HERE, '..', d) for d in ('probes', 'decode')]
import bedrock_crypto as crypto

LISTEN = ('127.0.0.1', int(sys.argv[1]) if len(sys.argv) > 1 else 19140)
TARGET = ('127.0.0.1', int(sys.argv[2]) if len(sys.argv) > 2 else 19133)
MAGIC = bytes.fromhex('00ffff00fefefefefdfdfdfd12345678')
GUID = 0x5EED1234ABCD

NAMES = {}
for path in glob.glob(os.path.join(HERE, '..', 'reference', 'bedrock_docs', '*Packet.json')):
    doc = json.load(open(path))
    pid = doc.get('$metaProperties', {}).get('[cereal:packet]')
    if pid is not None:
        NAMES[pid] = doc['title']

LOG_DIR = os.path.join(HERE, '..', 'captures', 'bedrock_log', time.strftime('%Y%m%d_%H%M%S'))
os.makedirs(LOG_DIR, exist_ok=True)
LOG = open(os.path.join(LOG_DIR, 'traffic.log'), 'w', encoding='utf-8')
START = time.time()
COUNTER = [0]

def log(text):
    line = '%8.3f %s' % (time.time() - START, text)
    print(line)
    LOG.write(line + '\n'); LOG.flush()

def record(direction, packet):
    header, p = uv(packet, 0)
    pid = header & 0x3FF
    COUNTER[0] += 1
    open(os.path.join(LOG_DIR, '%05d_%s_%d.bin' % (COUNTER[0], 'cs' if direction == 'C->S' else 'sc', pid)), 'wb').write(packet[p:])
    log('%s %4d %-44s %8d  %s' % (direction, pid, NAMES.get(pid, '?'), len(packet) - p, packet[p:p + 32].hex(' ')))
    return pid

def uv(b, p):
    r = s = 0
    while True:
        x = b[p]; p += 1; r |= (x & 0x7F) << s; s += 7
        if not x & 0x80:
            return r, p

def uvarint(v):
    out = bytearray()
    while True:
        b = v & 0x7F; v >>= 7
        if v:
            out.append(b | 0x80)
        else:
            out.append(b); return bytes(out)

def address(addr):
    host, port = addr
    return b'\x04' + bytes(~int(x) & 0xFF for x in host.split('.')) + struct.pack('>H', port)

class Peer:
    def __init__(self, sock, addr, mtu):
        self.sock, self.addr, self.mtu = sock, addr, mtu
        self.seq = self.reliable = self.order = self.split_id = 0
        self.seen = set(); self.pending = {}; self.next_order = 0; self.splits = {}
        self.sent = {}

    def raw(self, data):
        self.sock.sendto(data, self.addr)

    def send(self, body, reliable=True):
        limit = self.mtu - 28 - 4 - 24
        parts = [body[i:i + limit] for i in range(0, len(body), limit)] or [b'']
        order = self.order; self.order += 1
        sid = self.split_id; self.split_id = (self.split_id + 1) & 0xFFFF
        for index, part in enumerate(parts):
            split = len(parts) > 1
            frame = bytes([(3 << 5) | (0x10 if split else 0)]) + struct.pack('>H', len(part) * 8)
            frame += struct.pack('<I', self.reliable)[:3] + struct.pack('<I', order)[:3] + b'\x00'
            self.reliable += 1
            if split:
                frame += struct.pack('>IHI', len(parts), sid, index)
            frame += part
            datagram = bytes([0x84]) + struct.pack('<I', self.seq)[:3] + frame
            self.sent[self.seq] = (datagram, time.time())
            self.seq += 1
            self.raw(datagram)

    def resend(self, seq):
        if seq in self.sent:
            datagram = self.sent.pop(seq)[0]
            datagram = datagram[:1] + struct.pack('<I', self.seq)[:3] + datagram[4:]
            self.sent[self.seq] = (datagram, time.time())
            self.seq += 1
            self.raw(datagram)

    def tick(self):
        now = time.time()
        for seq in [s for s, (_, t) in self.sent.items() if now - t > 1.0]:
            self.resend(seq)

    def receive(self, d):
        out = []
        if d[0] & 0x40 or d[0] & 0x20:
            nack = bool(d[0] & 0x20)
            count = struct.unpack('>H', d[1:3])[0]; p = 3
            for _ in range(count):
                single = d[p]; p += 1
                first = int.from_bytes(d[p:p + 3], 'little'); p += 3
                last = first
                if not single:
                    last = int.from_bytes(d[p:p + 3], 'little'); p += 3
                for seq in range(first, last + 1):
                    if nack: self.resend(seq)
                    else: self.sent.pop(seq, None)
            return out
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
                if reliable in self.seen: continue
                self.seen.add(reliable)
            if split:
                count, sid, idx = split
                parts = self.splits.setdefault(sid, {})
                parts[idx] = body
                if len(parts) < count: continue
                body = b''.join(parts[i] for i in range(count)); del self.splits[sid]
            if order is None:
                out.append(body); continue
            self.pending[order] = body
            while self.next_order in self.pending:
                out.append(self.pending.pop(self.next_order)); self.next_order += 1
        return out

class Proxy:
    def __init__(self):
        self.client_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.client_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
        self.client_sock.bind(LISTEN)
        self.client_sock.setblocking(False)
        self.server_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8 << 20)
        self.server_sock.setblocking(False)
        self.client = None
        self.server = None
        self.client_compression = False
        self.server_compression = False
        self.key = crypto.Key()
        self.encrypt = self.decrypt = self.secret = None
        self.send_counter = 0

    def connect_server(self, mtu):
        self.server_sock.setblocking(True); self.server_sock.settimeout(3)
        self.server_sock.sendto(b'\x05' + MAGIC + b'\x0b' + b'\x00' * (mtu - 28 - 18), TARGET)
        reply = self.server_sock.recvfrom(4096)[0]
        server_mtu = struct.unpack('>H', reply[26:28])[0]
        self.server_guid = int.from_bytes(os.urandom(7), 'big')
        self.server_sock.sendto(b'\x07' + MAGIC + address(TARGET) + struct.pack('>H', server_mtu) + struct.pack('>Q', self.server_guid), TARGET)
        self.server_sock.recvfrom(4096)
        self.server_sock.setblocking(False)
        self.server = Peer(self.server_sock, TARGET, server_mtu)
        self.server_ready = False
        self.queued = []
        self.server.send(b'\x09' + struct.pack('>QQ', self.server_guid, int(time.time() * 1000)) + b'\x00')
        log('-- proxy connected to official server, mtu %d' % server_mtu)

    def offline(self, d, addr):
        if d[0] in (0x01, 0x02):
            motd = 'MCPE;logging proxy;2193;1.26.52;0;10;%d;proxy;Survival;1;%d;%d;0;' % (GUID, LISTEN[1], LISTEN[1] + 1)
            self.client_sock.sendto(b'\x1c' + d[1:9] + struct.pack('>Q', GUID) + MAGIC + struct.pack('>H', len(motd)) + motd.encode(), addr)
        elif d[0] == 0x05:
            mtu = min(len(d) + 28, 1492)
            self.client_sock.sendto(b'\x06' + MAGIC + struct.pack('>Q', GUID) + b'\x00' + struct.pack('>H', mtu), addr)
        elif d[0] == 0x07:
            mtu = struct.unpack('>H', d[17 + 7:17 + 9])[0]
            self.client = Peer(self.client_sock, addr, mtu)
            self.client_compression = self.server_compression = False
            self.encrypt = self.decrypt = None
            self.send_counter = 0
            self.client_sock.sendto(b'\x08' + MAGIC + struct.pack('>Q', GUID) + address(addr) + struct.pack('>H', mtu) + b'\x00', addr)
            log('-- game client %s:%d opened, mtu %d' % (addr[0], addr[1], mtu))
            self.connect_server(mtu)

    def decode(self, payload, from_server):
        if from_server and self.decrypt:
            payload = self.decrypt.process(payload)[:-8]
        compressed = self.server_compression if from_server else self.client_compression
        if compressed:
            algorithm, payload = payload[0], payload[1:]
            if algorithm == 0x00:
                payload = zlib.decompress(payload, -15)
            elif algorithm != 0xFF:
                raise RuntimeError('compression %x' % algorithm)
        packets, p = [], 0
        while p < len(payload):
            n, p = uv(payload, p)
            packets.append(payload[p:p + n]); p += n
        return packets

    def to_server(self, packets):
        batch = b''.join(uvarint(len(p)) + p for p in packets)
        if self.server_compression:
            c = zlib.compressobj(6, zlib.DEFLATED, -15)
            batch = b'\x00' + c.compress(batch) + c.flush()
        if self.encrypt:
            checksum = hashlib.sha256(struct.pack('<Q', self.send_counter) + batch + self.secret).digest()[:8]
            self.send_counter += 1
            batch = self.encrypt.process(batch + checksum)
        self.server.send(b'\xfe' + batch)

    def to_client(self, packets):
        batch = b''.join(uvarint(len(p)) + p for p in packets)
        if self.client_compression:
            batch = b'\xff' + batch
        self.client.send(b'\xfe' + batch)

    def rewrite_login(self, packet):
        header, p = uv(packet, 0)
        protocol = packet[p:p + 4]; p += 4
        n, p = uv(packet, p); request = packet[p:p + n]
        first = struct.unpack('<I', request[:4])[0]
        identity = json.loads(request[4:4 + first])
        token_payload = {}
        if identity.get('Token'):
            token_payload = crypto.jwt_payload(identity['Token'])
        client_jwt = request[8 + first:].decode()
        client_payload = crypto.jwt_payload(client_jwt)
        client_payload['ServerAddress'] = '%s:%d' % TARGET
        now = int(time.time())
        token = crypto.jwt(self.key, {'sub': 'proxy', 'iat': now, 'exp': now + 86400, 'cpk': self.key.public_der_b64(), 'xid': token_payload.get('xid', ''), 'xname': token_payload.get('xname', client_payload.get('ThirdPartyName', 'proxy'))})
        chain = json.dumps({'AuthenticationType': 2, 'Token': token}).encode()
        client = crypto.jwt(self.key, client_payload).encode()
        request = struct.pack('<I', len(chain)) + chain + struct.pack('<I', len(client)) + client
        log('-- login re-signed for the official server: identity keys %s, client data %d fields' % (list(identity.keys()), len(client_payload)))
        return uvarint(header) + protocol + uvarint(len(request)) + request

    def from_client(self, msg):
        kind = msg[0]
        if os.environ.get('PROXY_TRACE'): log('-- client message 0x%02x len %d' % (kind, len(msg)))
        if kind == 0x09:
            self.client.send(b'\x10' + address(self.client.addr) + b'\x00\x00' + address(('0.0.0.0', 0)) * 20 + msg[9:17] + struct.pack('>Q', int(time.time() * 1000)))
        elif kind == 0x00:
            self.client.send(b'\x03' + msg[1:9] + struct.pack('>Q', int(time.time() * 1000)), reliable=False)
        elif kind == 0x15:
            log('-- game client disconnected')
            if self.server: self.server.send(b'\x15')
        elif kind == 0xFE:
            forward = []
            for packet in self.decode(msg[1:], False):
                pid = record('C->S', packet)
                forward.append(self.rewrite_login(packet) if pid == 1 else packet)
            if forward and self.server_ready: self.to_server(forward)
            elif forward: self.queued.append(forward)

    def from_server(self, msg):
        kind = msg[0]
        if os.environ.get('PROXY_TRACE'): log('-- server message 0x%02x len %d' % (kind, len(msg)))
        if kind == 0x10:
            self.server.send(b'\x13' + address(TARGET) + address(('0.0.0.0', 0)) * 20 + msg[-16:-8] + struct.pack('>Q', int(time.time() * 1000)))
            self.server_ready = True
            for batch in self.queued: self.to_server(batch)
            self.queued = []
            log('-- official server accepted connection, flushed queued game packets')
        elif kind == 0x00:
            self.server.send(b'\x03' + msg[1:9] + struct.pack('>Q', int(time.time() * 1000)), reliable=False)
        elif kind == 0x15:
            log('-- official server disconnected us')
        elif kind == 0xFE:
            forward = []
            settings = False
            for packet in self.decode(msg[1:], True):
                pid = record('S->C', packet)
                if pid == 3:
                    token = packet[uv(packet, uv(packet, 0)[1])[1]:].decode()
                    server_key = crypto.jwt_header(token)['x5u']
                    salt = base64.b64decode(crypto.jwt_payload(token)['salt'])
                    self.secret = hashlib.sha256(salt + self.key.shared_secret(server_key)).digest()
                    iv = self.secret[:12] + b'\x00\x00\x00\x02'
                    self.encrypt = crypto.CTR(self.secret, iv); self.decrypt = crypto.CTR(self.secret, iv)
                    self.to_server([uvarint(4)])
                    log('-- encryption with official server enabled (not forwarded to game)')
                    continue
                if pid == 143:
                    settings = True
                forward.append(packet)
            if forward: self.to_client(forward)
            if settings:
                self.client_compression = self.server_compression = True

    def run(self):
        log('-- logging proxy on %s:%d -> official server %s:%d, logs in %s' % (LISTEN + TARGET + (LOG_DIR,)))
        last = time.time()
        while True:
            ready, _, _ = select.select([self.client_sock, self.server_sock], [], [], 0.05)
            for sock in ready:
                while True:
                    try:
                        d, addr = sock.recvfrom(65535)
                    except (BlockingIOError, ConnectionResetError):
                        break
                    try:
                        if sock is self.client_sock:
                            if d[0] & 0x80 and self.client and addr == self.client.addr:
                                for msg in self.client.receive(d): self.from_client(msg)
                            elif not d[0] & 0x80:
                                self.offline(d, addr)
                        elif self.server:
                            for msg in self.server.receive(d): self.from_server(msg)
                    except Exception as e:
                        log('-- error handling datagram: %r ' % e + traceback.format_exc().replace(chr(10), ' | '))
            if time.time() - last > 0.25:
                last = time.time()
                if self.client: self.client.tick()
                if self.server: self.server.tick()

Proxy().run()
