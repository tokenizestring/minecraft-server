import struct, sys, glob, os

class R:
    def __init__(self, b): self.b, self.p = b, 0
    def u8(self): v = self.b[self.p]; self.p += 1; return v
    def bool(self): return self.u8() != 0
    def uv(self):
        r = s = 0
        while True:
            x = self.u8(); r |= (x & 0x7F) << s; s += 7
            if not x & 0x80: return r
    def zz(self):
        v = self.uv(); return (v >> 1) ^ -(v & 1)
    def f32(self): v = struct.unpack('<f', self.b[self.p:self.p + 4])[0]; self.p += 4; return v
    def i16(self): v = struct.unpack('<h', self.b[self.p:self.p + 2])[0]; self.p += 2; return v
    def i32(self): v = struct.unpack('<i', self.b[self.p:self.p + 4])[0]; self.p += 4; return v
    def u32(self): v = struct.unpack('<I', self.b[self.p:self.p + 4])[0]; self.p += 4; return v
    def u64(self): v = struct.unpack('<Q', self.b[self.p:self.p + 8])[0]; self.p += 8; return v
    def s(self): n = self.uv(); v = self.b[self.p:self.p + n]; self.p += n; return v.decode(errors='replace')
    def nbt_skip(self):
        def payload(t):
            if t == 1: self.u8()
            elif t == 2: self.i16()
            elif t == 3: self.zz()
            elif t == 4: self.uv()
            elif t == 5: self.f32()
            elif t == 6: self.p += 8
            elif t == 7: self.p += self.zz()
            elif t == 8: self.s()
            elif t == 9:
                inner = self.u8(); n = self.zz()
                for _ in range(n): payload(inner)
            elif t == 10:
                while True:
                    inner = self.u8()
                    if inner == 0: break
                    self.s(); payload(inner)
            elif t == 11:
                for _ in range(self.zz()): self.zz()
            elif t == 12:
                for _ in range(self.zz()): self.uv()
            else: raise ValueError('nbt %d' % t)
        t = self.u8(); self.s(); payload(t)

def parse(b):
    r = R(b); v = {}
    v['entity'] = r.zz(); v['runtime'] = r.uv(); v['gametype'] = r.zz()
    v['pos'] = (r.f32(), r.f32(), r.f32()); v['rot'] = (r.f32(), r.f32())
    v['seed'] = r.u64(); v['spawn_biome'] = r.i16(); v['biome_name'] = r.s(); v['dimension'] = r.zz()
    v['generator'] = r.zz(); v['gametype2'] = r.zz(); v['hardcore'] = r.bool(); v['difficulty'] = r.zz()
    v['spawn'] = (r.zz(), r.zz(), r.zz()); v['achievements_off'] = r.bool(); v['editor_type'] = r.zz()
    v['created_editor'] = r.bool(); v['exported_editor'] = r.bool(); v['day_stop'] = r.zz(); v['edu_offer'] = r.uv()
    v['edu_features'] = r.bool(); v['edu_product'] = r.s(); v['rain'] = r.f32(); v['lightning'] = r.f32()
    v['platform_locked'] = r.bool(); v['mp_intent'] = r.bool(); v['lan'] = r.bool(); v['xbox_bc'] = r.zz(); v['plat_bc'] = r.zz()
    v['commands'] = r.bool(); v['texture_required'] = r.bool()
    rules = []
    for _ in range(r.uv()):
        name = r.s(); editable = r.bool(); kind = r.uv()
        val = r.bool() if kind == 1 else r.i32() if kind == 2 else r.f32() if kind == 3 else None
        rules.append((name, kind, val))
    v['rules'] = len(rules); v['rule_kinds'] = sorted(set(k for _, k, _ in rules)); v['rule_sample'] = rules[:3]
    n = r.u32(); v['experiments'] = [(r.s(), r.bool()) for _ in range(n)]; v['ever_toggled'] = r.bool()
    v['bonus'] = r.bool(); v['map'] = r.bool(); v['permissions'] = r.zz(); v['tick_range'] = r.i32()
    v['ten_bools'] = [r.bool() for _ in range(10)]
    v['base_version'] = r.s(); v['width'] = r.i32(); v['depth'] = r.i32(); v['nether'] = r.bool()
    v['edu_uri'] = (r.s(), r.s()); v['force_exp'] = r.bool(); v['chat_restrict'] = r.u8(); v['no_interact'] = r.bool()
    v['editor_policy'] = r.zz(); v['anon_drops'] = r.bool()
    v['level_id'] = r.s(); v['level_name'] = r.s(); v['template_identity'] = r.s(); v['trial'] = r.bool()
    v['rewind'] = r.zz(); v['server_block_breaking'] = r.bool(); v['time'] = r.u64(); v['enchant_seed'] = r.zz()
    blocks = r.uv()
    for _ in range(blocks): r.s(); r.nbt_skip()
    v['block_properties'] = blocks
    v['correlation'] = r.s(); v['item_stack_net'] = r.bool(); v['server_version'] = r.s()
    at = r.p; r.nbt_skip(); v['player_props_bytes'] = b[at:r.p].hex(' ')[:60]
    v['checksum'] = r.u64(); v['template_uuid'] = (r.u64(), r.u64()); v['client_gen'] = r.bool(); v['hashes'] = r.bool()
    v['sound_auth'] = r.bool()
    v['tail'] = b[r.p:r.p + 8].hex(' ')
    r.p += 8
    v['consumed'] = '%d of %d' % (r.p, len(b))
    v['exact'] = (r.p == len(b))
    return v

for path in sys.argv[1:]:
    for k, val in parse(open(path, 'rb').read()).items():
        print('%-22s %s' % (k, val))
