import struct, sys, json, os

HERE = os.path.dirname(os.path.abspath(__file__))
DATA_IDS = json.load(open(os.path.join(HERE, '..', 'reference', 'bedrock_docs', 'ActorDataIDs.json'))).get('enum', []) if os.path.exists(os.path.join(HERE, '..', 'reference', 'bedrock_docs', 'ActorDataIDs.json')) else []

class R:
    def __init__(self, b): self.b, self.p = b, 0
    def take(self, n): v = self.b[self.p:self.p + n]; self.p += n; return v
    def u8(self): return self.take(1)[0]
    def uv(self):
        r = s = 0
        while True:
            x = self.u8(); r |= (x & 0x7F) << s; s += 7
            if not x & 0x80: return r
    def zz(self): v = self.uv(); return (v >> 1) ^ -(v & 1)
    def f32(self): return struct.unpack('<f', self.take(4))[0]
    def s(self): return self.take(self.uv()).decode(errors='replace')

def nbt(r):
    def payload(t):
        if t == 1: return r.u8()
        if t == 2: return struct.unpack('<h', r.take(2))[0]
        if t == 3: return r.zz()
        if t == 4: return r.zz()
        if t == 5: return r.f32()
        if t == 6: return struct.unpack('<d', r.take(8))[0]
        if t == 7: return r.take(r.zz())
        if t == 8: return r.s()
        if t == 9:
            inner = r.u8(); return [payload(inner) for _ in range(r.zz())]
        if t == 10:
            out = {}
            while True:
                inner = r.u8()
                if inner == 0: return out
                key = r.s(); out[key] = payload(inner)
        raise ValueError('nbt %d' % t)
    t = r.u8(); r.s(); return payload(t)

def data_item(r):
    key = r.uv(); control = r.uv(); kind = r.u8()
    if kind == 0: value = r.u8()
    elif kind == 1: value = struct.unpack('<h', r.take(2))[0]
    elif kind == 2: value = r.zz()
    elif kind == 3: value = r.f32()
    elif kind == 4: value = r.s()
    elif kind == 5: value = nbt(r)
    elif kind == 6: value = (r.zz(), r.zz(), r.zz())
    elif kind == 7: value = r.zz()
    elif kind == 8: value = (r.f32(), r.f32(), r.f32())
    else: raise ValueError('data item kind %d' % kind)
    return key, control, kind, value

def parse(b):
    r = R(b); v = {}
    v['uuid'] = r.take(16).hex(); v['name'] = r.s(); v['runtime'] = r.uv(); v['chat_id'] = r.s()
    v['pos'] = (r.f32(), r.f32(), r.f32()); v['vel'] = (r.f32(), r.f32(), r.f32()); v['rot'] = (r.f32(), r.f32()); v['head'] = r.f32()
    item = struct.unpack('<h', r.take(2))[0]; stack = struct.unpack('<H', r.take(2))[0]
    v['item'] = (item, stack, r.uv(), r.uv(), r.uv(), r.s())
    v['gametype'] = r.zz()
    v['data'] = [data_item(r) for _ in range(r.uv())]
    v['int_props'] = [(r.uv(), r.zz()) for _ in range(r.uv())]
    v['float_props'] = [(r.uv(), r.f32()) for _ in range(r.uv())]
    v['raw_id'] = struct.unpack('<q', r.take(8))[0]; v['permissions'] = r.u8(); v['command_permissions'] = r.u8()
    v['layers'] = [(struct.unpack('<H', r.take(2))[0], struct.unpack('<I', r.take(4))[0], struct.unpack('<I', r.take(4))[0], r.f32(), r.f32(), r.f32()) for _ in range(r.uv())]
    v['links'] = r.uv(); v['device'] = r.s(); v['platform'] = struct.unpack('<i', r.take(4))[0]
    v['consumed'] = '%d of %d' % (r.p, len(b))
    return v

if __name__ == '__main__':
    v = parse(open(sys.argv[1], 'rb').read())
    for key, val in v.items():
        if key != 'data': print('%-20s %s' % (key, val))
    for key, control, kind, value in v['data']:
        name = DATA_IDS[key] if key < len(DATA_IDS) else '?'
        print('   data %3d %-32s control %d kind %d value %r' % (key, name, control, kind, value if not isinstance(value, (bytes, dict)) else str(value)[:60]))
