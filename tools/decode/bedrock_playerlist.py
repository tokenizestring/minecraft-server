import struct, sys, os
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from bedrock_addplayer import R

def image(r):
    w = struct.unpack('<I', r.take(4))[0]; h = struct.unpack('<I', r.take(4))[0]
    n = r.uv(); data = r.take(n)
    return (w, h, n)

def skin(r):
    v = {}
    v['id'] = r.s(); v['playfab'] = r.s(); v['patch'] = r.s()[:60]
    v['image'] = image(r)
    v['animations'] = []
    for _ in range(struct.unpack('<I', r.take(4))[0] if os.environ.get('ANIM_U32') else r.uv()):
        v['animations'].append((image(r), r.uv(), r.f32(), r.uv()))
    v['cape'] = image(r)
    v['geometry'] = len(r.s()); v['engine'] = r.s(); v['animation'] = len(r.s())
    v['cape_id'] = r.s(); v['full_id'] = r.s(); v['arm'] = r.s() if os.environ.get('ARM_STRING') else r.u8()
    v['color'] = r.s() if os.environ.get('COLOR_STRING') else struct.unpack('<i', r.take(4))[0]
    pieces = r.uv()
    v['pieces'] = pieces
    for _ in range(pieces):
        r.s(); struct.unpack('<I', r.take(4)); r.take(16); r.u8(); r.s()
    tints = r.uv(); v['tints'] = tints
    for _ in range(tints):
        r.s(); [r.s() for _ in range(r.uv())]
    v['premium'] = r.u8(); v['persona'] = r.u8(); v['cape_classic'] = r.u8(); v['primary'] = r.u8(); v['overrides'] = r.u8()
    v['trusted'] = r.s(); v['profile_hash'] = r.s()
    return v

def parse(b):
    r = R(b); out = {}
    count = r.uv(); out['count'] = count
    entries = []
    for _ in range(count):
        e = {}
        e['control'] = r.uv(); e['action'] = r.u8(); e['uuid'] = r.take(16).hex()
        e['unique_id'] = r.zz(); e['name'] = r.s(); e['xuid'] = r.s(); e['platform_id'] = r.s()
        e['build'] = struct.unpack('<i', r.take(4))[0]
        e['skin'] = skin(r)
        e['teacher'] = r.u8(); e['host'] = r.u8(); e['subclient'] = r.u8()
        e['color'] = struct.unpack('<i', r.take(4))[0]
        entries.append(e)
    out['entries'] = entries
    out['tail'] = b[r.p:].hex(' ')
    out['consumed'] = '%d of %d' % (r.p, len(b))
    return out

if __name__ == '__main__':
    out = parse(open(sys.argv[1], 'rb').read())
    for e in out['entries']:
        for k, v in e.items():
            if k != 'skin': print('%-12s %s' % (k, v))
        for k, v in e['skin'].items():
            print('   skin.%-12s %s' % (k, v))
    print('tail', out['tail'], 'consumed', out['consumed'])
