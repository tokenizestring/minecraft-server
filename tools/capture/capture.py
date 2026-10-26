import socket, struct, time, sys, os

HOST = '127.0.0.1'
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 25566
PROTOCOL = int(sys.argv[2]) if len(sys.argv) > 2 else 777
TAG = sys.argv[3] if len(sys.argv) > 3 else '26_3'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'captures', 'capture_' + TAG)

CONFIG_FINISH = 3
CONFIG_KEEP_ALIVE = 4
CONFIG_PING = 5
CONFIG_KNOWN_PACKS_OUT = 15
CONFIG_KNOWN_PACKS_IN = 7

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
            return r, p

def mstr(t):
    t = t.encode()
    return varint(len(t)) + t

sock = socket.create_connection((HOST, PORT), timeout=20)
rx = b''

def send(pid, body=b''):
    p = varint(pid) + body
    sock.sendall(varint(len(p)) + p)

def recv():
    global rx
    while True:
        try:
            n, p = rvarint(rx, 0)
            if len(rx) >= p + n:
                frame = rx[p:p + n]; raw = rx[:p + n]; rx = rx[p + n:]
                pid, q = rvarint(frame, 0)
                return pid, frame[q:], frame
        except IndexError:
            pass
        d = sock.recv(1 << 20)
        if not d:
            raise SystemExit('closed, buffered %d' % len(rx))
        rx += d

os.makedirs(OUT, exist_ok=True)
send(0, varint(PROTOCOL) + mstr('localhost') + struct.pack('>H', PORT) + varint(2))
send(0, mstr('capture') + b'\x22' * 16)

pid, body, _ = recv()
assert pid == 2, 'login got %d %r' % (pid, body[:80])
open(os.path.join(OUT, 'login_success.bin'), 'wb').write(body)
send(3)

blob = bytearray()
names = []
while True:
    pid, body, frame = recv()
    if pid == CONFIG_KEEP_ALIVE:
        send(CONFIG_KEEP_ALIVE, body); continue
    if pid == CONFIG_PING:
        send(CONFIG_PING, body); continue
    blob += varint(len(frame)) + frame
    label = ''
    if pid == 7:
        l, q = rvarint(body, 0); label = body[q:q + l].decode()
    names.append((pid, len(body), label))
    if pid == CONFIG_KNOWN_PACKS_OUT:
        print('known packs', body)
        send(CONFIG_KNOWN_PACKS_IN, body)
    if pid == CONFIG_FINISH:
        break

open(os.path.join(OUT, 'registry.bin'), 'wb').write(blob)
for n in names:
    print('config', hex(n[0]), n[1], n[2])
print('blob bytes', len(blob))

send(CONFIG_FINISH)
seen = {}
deadline = time.time() + 6
while time.time() < deadline:
    pid, body, _ = recv()
    seen[pid] = seen.get(pid, 0) + 1
    if seen[pid] <= 2:
        open(os.path.join(OUT, 'play_%02x_%d.bin' % (pid, seen[pid])), 'wb').write(body)
print('play', {hex(k): v for k, v in sorted(seen.items())})
sock.close()
