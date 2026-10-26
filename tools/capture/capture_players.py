import socket, struct, time, sys, os, json, threading

HOST = '127.0.0.1'
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 25566
PROTOCOL = int(sys.argv[2]) if len(sys.argv) > 2 else 777
TAG = sys.argv[3] if len(sys.argv) > 3 else 'java_players'
ROOT = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(ROOT, '..', 'captures', 'capture_' + TAG)
REPORT = os.path.join(ROOT, '..', 'reference', 'gen_26_3' if PROTOCOL == 777 else 'gen_26_2', 'reports', 'packets.json')

ids = json.load(open(REPORT))
NAMES = {v['protocol_id']: k.split(':')[1] for k, v in ids['play']['clientbound'].items()}
SERVERBOUND = {k.split(':')[1]: v['protocol_id'] for k, v in ids['play']['serverbound'].items()}
CLIENTBOUND = {v: k for k, v in NAMES.items()}
CONFIG_IN = {k.split(':')[1]: v['protocol_id'] for k, v in ids['configuration']['clientbound'].items()}
CONFIG_OUT = {k.split(':')[1]: v['protocol_id'] for k, v in ids['configuration']['serverbound'].items()}

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

class Player:
    def __init__(self, name):
        self.name = name
        self.sock = socket.create_connection((HOST, PORT), timeout=20)
        self.rx = b''
        self.log = []
        self.alive = True
        self.position = (0.0, 0.0, 0.0)

    def send(self, pid, body=b''):
        p = varint(pid) + body
        self.sock.sendall(varint(len(p)) + p)

    def recv(self):
        while True:
            try:
                n, p = rvarint(self.rx, 0)
                if len(self.rx) >= p + n:
                    frame = self.rx[p:p + n]; self.rx = self.rx[p + n:]
                    pid, q = rvarint(frame, 0)
                    return pid, frame[q:]
            except IndexError:
                pass
            d = self.sock.recv(1 << 20)
            if not d: raise EOFError
            self.rx += d

    def join(self):
        self.send(0, varint(PROTOCOL) + mstr('localhost') + struct.pack('>H', PORT) + varint(2))
        self.send(0, mstr(self.name) + bytes(16))
        pid, body = self.recv(); assert pid == 2, pid
        self.send(3)
        while True:
            pid, body = self.recv()
            if pid == CONFIG_IN['keep_alive']: self.send(CONFIG_OUT['keep_alive'], body)
            elif pid == CONFIG_IN['ping']: self.send(CONFIG_OUT['pong'], body)
            elif pid == CONFIG_IN['select_known_packs']: self.send(CONFIG_OUT['select_known_packs'], body)
            elif pid == CONFIG_IN['finish_configuration']: break
        self.send(CONFIG_OUT['finish_configuration'])
        self.sock.settimeout(0.2)

    def pump(self, seconds, record):
        deadline = time.time() + seconds
        while time.time() < deadline:
            try:
                pid, body = self.recv()
            except socket.timeout:
                continue
            name = NAMES.get(pid, '?')
            if name == 'keep_alive':
                self.send(SERVERBOUND['keep_alive'], body)
            elif name == 'player_position':
                tid, q = rvarint(body, 0)
                self.position = struct.unpack('>ddd', body[q:q + 24])
                extra = body[q:q + 24] + body[q + 48:q + 56] if PROTOCOL >= 777 else b''
                self.send(SERVERBOUND['accept_teleportation'], varint(tid) + extra)
            if name == 'chunk_batch_finished':
                self.send(SERVERBOUND['chunk_batch_received'], struct.pack('>f', 25.0))
            if name == 'login':
                self.send(SERVERBOUND['player_loaded'])
            if record:
                self.log.append((time.time(), pid, name, body))

def main():
    os.makedirs(OUT, exist_ok=True)
    a = Player('Watcher')
    a.join()
    for _ in range(10):
        a.pump(0.3, False)
        a.send(SERVERBOUND['client_tick_end'])
    print("watcher spawn", a.position)
    b = Player('Walker')
    b.join()
    stop = threading.Event()
    def watch():
        while not stop.is_set():
            a.send(SERVERBOUND['move_player_pos_rot'], struct.pack('>dddffB', a.position[0], a.position[1], a.position[2], 0.0, 0.0, 1))
            a.send(SERVERBOUND['client_tick_end'])
            a.pump(0.3, True)
    t = threading.Thread(target=watch); t.start()
    b.pump(2, False)
    x, y, z = b.position
    print("walker spawn", b.position)
    for step in range(10):
        b.send(SERVERBOUND['move_player_pos_rot'], struct.pack('>dddffB', x + step * 0.3, y, z, step * 15.0, 5.0, 1))
        b.send(SERVERBOUND['client_tick_end'])
        b.pump(0.25, False)
    b.pump(1, False)
    b.sock.close()
    time.sleep(2)
    stop.set(); t.join()
    for index, (when, pid, name, body) in enumerate(a.log):
        open(os.path.join(OUT, '%04d_%02x_%s.bin' % (index, pid, name)), 'wb').write(body)
        if name not in ('set_time', 'keep_alive', 'level_chunk_with_light', 'light_update', 'chunks_biomes', 'set_chunk_cache_center'):
            print('%03d %-28s %5d %s' % (index, name, len(body), body[:48].hex(' ')))

main()
