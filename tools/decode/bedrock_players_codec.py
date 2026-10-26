import struct, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from bedrock_addplayer import R, nbt

def uv(v):
    v &= (1 << 64) - 1
    out = bytearray()
    while True:
        b = v & 0x7F; v >>= 7
        if v: out.append(b | 0x80)
        else:
            out.append(b); return bytes(out)

def zz(v): return uv((v << 1) ^ (v >> 63))
def st(t): t = t if isinstance(t, bytes) else t.encode(); return uv(len(t)) + t
def rs(r): return r.take(r.uv())

def read_image(r): return (struct.unpack('<I', r.take(4))[0], struct.unpack('<I', r.take(4))[0], rs(r))
def write_image(i): return struct.pack('<II', i[0], i[1]) + st(i[2])

def read_skin(r):
    s = {}
    s['id'] = rs(r); s['playfab'] = rs(r); s['patch'] = rs(r); s['image'] = read_image(r)
    s['animations'] = [(read_image(r), r.uv(), r.f32(), r.uv()) for _ in range(r.uv())]
    s['cape'] = read_image(r); s['geometry'] = rs(r); s['engine'] = rs(r); s['animation'] = rs(r)
    s['cape_id'] = rs(r); s['full_id'] = rs(r); s['arm'] = r.u8(); s['color'] = r.take(4)
    s['pieces'] = r.uv(); assert s['pieces'] == 0
    s['tints'] = r.uv(); assert s['tints'] == 0
    s['flags'] = r.take(5); s['trusted'] = rs(r); s['profile_hash'] = rs(r)
    return s

def write_skin(s):
    out = st(s['id']) + st(s['playfab']) + st(s['patch']) + write_image(s['image'])
    out += uv(len(s['animations'])) + b''.join(write_image(a[0]) + uv(a[1]) + struct.pack('<f', a[2]) + uv(a[3]) for a in s['animations'])
    out += write_image(s['cape']) + st(s['geometry']) + st(s['engine']) + st(s['animation'])
    out += st(s['cape_id']) + st(s['full_id']) + bytes([s['arm']]) + s['color'] + uv(0) + uv(0)
    out += s['flags'] + st(s['trusted']) + st(s['profile_hash'])
    return out

def read_list(b):
    r = R(b); e = {}
    assert r.uv() == 1
    e['control'] = r.uv(); e['action'] = r.u8(); e['uuid'] = r.take(16); e['unique'] = r.zz()
    e['name'] = rs(r); e['xuid'] = rs(r); e['platform_id'] = rs(r); e['build'] = struct.unpack('<i', r.take(4))[0]
    e['skin'] = read_skin(r); e['flags'] = r.take(3); e['color'] = struct.unpack('<i', r.take(4))[0]
    assert r.p == len(b), (r.p, len(b))
    return e

def write_list(e):
    return uv(1) + uv(e['control']) + bytes([e['action']]) + e['uuid'] + zz(e['unique']) + st(e['name']) + st(e['xuid']) + st(e['platform_id']) + struct.pack('<i', e['build']) + write_skin(e['skin']) + e['flags'] + struct.pack('<i', e['color'])

def read_add(b):
    r = R(b); v = {}
    v['uuid'] = r.take(16); v['name'] = rs(r); v['runtime'] = r.uv(); v['chat_id'] = rs(r)
    v['motion'] = r.take(36)
    start = r.p
    struct.unpack('<h', r.take(2)); r.take(2); r.uv(); r.uv(); r.uv(); r.s()
    v['item'] = b[start:r.p]
    v['gametype'] = r.zz()
    count = r.uv(); v['data'] = []
    for _ in range(count):
        key = r.uv(); control = r.uv(); kind = r.u8(); vs = r.p
        if kind == 4: value = rs(r)
        else:
            if kind == 0: r.u8()
            elif kind == 1: r.take(2)
            elif kind in (2, 7): r.uv()
            elif kind == 3: r.take(4)
            elif kind == 5: nbt(r)
            elif kind == 6: r.uv(); r.uv(); r.uv()
            elif kind == 8: r.take(12)
            value = b[vs:r.p]
        v['data'].append((key, control, kind, value))
    rest = r.p
    v['props'] = r.take(0)
    for _ in range(2):
        n = r.uv()
        for _ in range(n): r.uv(); r.take(4) if False else r.uv()
    v['props'] = b[rest:r.p]
    v['raw_id'] = struct.unpack('<q', r.take(8))[0]
    start = r.p
    r.take(2)
    for _ in range(r.uv()): r.take(22)
    v['abilities'] = b[start:r.p]
    v['links'] = r.uv(); v['device'] = rs(r); v['platform'] = struct.unpack('<i', r.take(4))[0]
    assert r.p == len(b), (r.p, len(b))
    return v

def write_add(v):
    out = v['uuid'] + st(v['name']) + uv(v['runtime']) + st(v['chat_id']) + v['motion'] + v['item'] + zz(v['gametype'])
    out += uv(len(v['data']))
    for key, control, kind, value in v['data']:
        out += uv(key) + uv(control) + bytes([kind]) + (st(value) if kind == 4 else value)
    out += v['props'] + struct.pack('<q', v['raw_id']) + v['abilities'] + uv(v['links']) + st(v['device']) + struct.pack('<i', v['platform'])
    return out

if __name__ == '__main__':
    for f in sys.argv[1:]:
        b = open(f, 'rb').read()
        kind = os.path.basename(f).split('_')[-1].split('.')[0]
        again = write_list(read_list(b)) if kind == '63' else write_add(read_add(b))
        print(f, 'round trip', again == b, len(b))
