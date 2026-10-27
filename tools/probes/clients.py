import zlib, socket, struct, time, json, os, sys, threading, base64

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path[:0] = [os.path.join(HERE, '..', d) for d in ('probes', 'decode')]
import bedrock_probe as bp
import bedrock_addplayer
import bedrock_playerlist

HOST = '127.0.0.1'

def varint(v):
    v &= 0xFFFFFFFF
    out = bytearray()
    while True:
        b = v & 0x7F; v >>= 7
        if v: out.append(b | 0x80)
        else:
            out.append(b); return bytes(out)

def rvarint(b, p):
    r = s = 0
    while True:
        x = b[p]; p += 1
        r |= (x & 0x7F) << s; s += 7
        if not x & 0x80: return r, p

def mstr(t):
    t = t.encode(); return varint(len(t)) + t

def packet_ids(tag):
    d = json.load(open(os.path.join(HERE, '..', 'reference', 'gen_' + tag, 'reports', 'packets.json')))
    cb = {k.split(':')[1]: v['protocol_id'] for k, v in d['play']['clientbound'].items()}
    sb = {k.split(':')[1]: v['protocol_id'] for k, v in d['play']['serverbound'].items()}
    return cb, sb

class Java:
    def __init__(self, name, protocol):
        self.name, self.protocol = name, protocol
        self.sock = socket.create_connection((HOST, 25565), timeout=10)
        self.rx = b''; self.zip = False; self.login = True; self.log = []; self.lock = threading.Lock(); self.running = True
        if protocol == 47:
            self.cb = {'add': 0x0C, 'move': 0x18, 'remove': 0x13, 'list': 0x38}
            self.sb = {'move': 0x06}
        else:
            cb, sb = packet_ids('26_3' if protocol == 777 else '26_2')
            self.cb = {'add': cb['add_entity'], 'move': cb['teleport_entity'], 'remove': cb['remove_entities'], 'list': cb['player_info_update']}
            self.sb = {'move': sb['move_player_pos_rot']}

    def send(self, pid, body=b''):
        p = varint(pid) + body
        if self.zip: p = b'\x00' + p
        self.sock.sendall(varint(len(p)) + p)

    def recv(self):
        while True:
            try:
                n, p = rvarint(self.rx, 0)
                if len(self.rx) >= p + n:
                    frame = self.rx[p:p + n]; self.rx = self.rx[p + n:]
                    if self.zip:
                        size, q = rvarint(frame, 0)
                        frame = zlib.decompress(frame[q:]) if size else frame[q:]
                    pid, q = rvarint(frame, 0)
                    if self.login and pid == 3:
                        self.zip = True; continue
                    if self.login and pid == 2: self.login = False
                    return pid, frame[q:]
            except IndexError:
                pass
            d = self.sock.recv(1 << 20)
            if not d: raise EOFError
            self.rx += d

    def join(self):
        self.send(0, varint(self.protocol) + mstr('localhost') + struct.pack('>H', 25565) + varint(2))
        self.send(0, mstr(self.name) + (bytes(16) if self.protocol >= 764 else b''))
        pid, body = self.recv(); assert pid == 2, (self.name, pid)
        if self.protocol >= 764:
            self.send(3)
            while True:
                pid, body = self.recv()
                if pid == 3: break
            self.send(3)
        threading.Thread(target=self.pump, daemon=True).start()

    def pump(self):
        self.sock.settimeout(0.2)
        while self.running:
            try:
                pid, body = self.recv()
            except socket.timeout:
                continue
            except (EOFError, OSError):
                return
            with self.lock:
                self.log.append((pid, body))

    def entity_ids(self, kind):
        with self.lock:
            return [rvarint(body, 0)[0] for pid, body in self.log if pid == self.cb[kind]]

    def removed_ids(self):
        out = []
        with self.lock:
            for pid, body in self.log:
                if pid == self.cb['remove']:
                    n, p = rvarint(body, 0)
                    for _ in range(n):
                        v, p = rvarint(body, p); out.append(v)
        return out

    def move(self, x, y, z, yaw):
        if self.protocol == 47:
            self.send(0x06, struct.pack('>dddff?', x, y, z, yaw, 0.0, True))
        else:
            self.send(self.sb['move'], struct.pack('>dddffB', x, y, z, yaw, 0.0, 1))

    def close(self):
        self.running = False
        self.sock.close()

class Bedrock:
    def __init__(self, name):
        self.name = name
        self.client = bp.Client()
        self.log = []; self.lock = threading.Lock(); self.running = True

    def join(self):
        c = self.client
        bp.connect(c)
        c.game([bp.uvarint(193) + struct.pack('>i', bp.PROTOCOL)]); c.messages(1); c.compression = True
        chain = json.dumps({'AuthenticationType': 2, 'Token': bp.jwt({'xname': self.name, 'xid': '2535411111111111'})})
        skin = base64.b64encode(bytes([0x80, 0x40, 0x20, 0xFF]) * 64 * 64).decode()
        client = bp.jwt({'ThirdPartyName': self.name, 'SkinId': 'probe_skin', 'SkinData': skin, 'SkinImageWidth': 64, 'SkinImageHeight': 64, 'SkinResourcePatch': base64.b64encode(b'{"geometry":{"default":"geometry.humanoid.custom"}}').decode(), 'SkinGeometryData': '', 'SkinGeometryDataEngineVersion': base64.b64encode(b'0.0.0').decode(), 'ArmSize': 'wide', 'PremiumSkin': False, 'DeviceOS': 7})
        request = struct.pack('<I', len(chain)) + chain.encode() + struct.pack('<I', len(client)) + client.encode()
        c.game([bp.uvarint(1) + struct.pack('>i', bp.PROTOCOL) + bp.uvarint(len(request)) + request], compress=True)
        c.messages(1)
        c.game([bp.uvarint(8) + bp.uvarint(2) + bp.uvarint(19) + b'downloadingfinished']); c.messages(1)
        c.game([bp.uvarint(8) + bp.uvarint(3) + bp.uvarint(25) + b'resourcepackstackfinished'])
        games = [g for m in c.messages(2) if m[0] == 0xFE for g in c.decode_game(m)]
        self.log.extend(games)
        start = [g for g in games if g[0] == 11][0][1]
        self.runtime, _ = rvarint(start, rvarint(start, 0)[1])
        c.game([bp.uvarint(113) + bp.uvarint(self.runtime)])
        threading.Thread(target=self.pump, daemon=True).start()

    def pump(self):
        while self.running:
            try:
                for m in self.client.messages(0.3):
                    if m[0] == 0xFE:
                        with self.lock:
                            self.log.extend(self.client.decode_game(m))
            except OSError:
                return

    def packets(self, pid):
        with self.lock:
            return [body for p, body in self.log if p == pid]

    def move(self, x, y, z, yaw):
        self.client.game([bp.uvarint(144) + struct.pack('<5f', 0.0, yaw, x, y + 1.62, z) + b'\x00' * 40])

    def close(self):
        self.running = False
        self.client.raw(b'\x84' + struct.pack('<I', self.client.seq)[:3] + bytes([0x60]) + struct.pack('>H', 8) + struct.pack('<I', self.client.reliable)[:3] + struct.pack('<I', self.client.order)[:3] + b'\x00' + b'\x15')

def move_exact(b):
    _, p = rvarint(b, 0); p += 24
    mode = b[p]; p += 2
    _, p = rvarint(b, p)
    present = b[p]; p += 1
    if present: p += 8
    _, p = rvarint(b, p)
    return p == len(b) and present == (mode == 2)
