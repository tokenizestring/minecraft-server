import time, sys, os, struct
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

def position(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((y & 0xFFF) << 26) | (z & 0x3FFFFFF))

def modern_position(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))

for protocol, tag in ((777, '26_3'), (776, '26_2')):
    cb, sb = packet_ids(tag)
    c = Java('Protect' + tag, protocol); c.join(); time.sleep(1.5)
    with c.lock: c.log.clear()
    c.send(sb['player_action'], varint(0) + modern_position(12, 59, 7) + b'\x01' + varint(42)); time.sleep(0.3)
    c.send(sb['use_item_on'], varint(0) + modern_position(12, 59, 7) + varint(1) + struct.pack('>fff', 0.5, 1.0, 0.5) + b'\x00\x00' + varint(300)); time.sleep(0.3)
    c.send(sb['use_item'], varint(0) + varint(301) + struct.pack('>ff', 0.0, 0.0)); time.sleep(0.5)
    with c.lock:
        acks = [rvarint(b, 0)[0] for p, b in c.log if p == cb['block_changed_ack']]
    check('%s break, place and use are acked back unchanged' % tag, acks == [42, 300, 301], acks)
    c.close()

c = Java('Protect18', 47); c.join(); time.sleep(1.0)
with c.lock: c.log.clear()
c.send(0x07, b'\x00' + position(12, 59, 7) + b'\x01'); time.sleep(0.3)
c.send(0x08, position(12, 59, 7) + b'\x01' + struct.pack('>h', 1) + b'\x40' + struct.pack('>h', 0) + b'\x00' + b'\x08\x10\x08'); time.sleep(0.5)
with c.lock:
    changes = []
    for p, b in c.log:
        if p == 0x23:
            v = struct.unpack('>Q', b[:8])[0]
            changes.append((((v >> 38) ^ (1 << 25)) - (1 << 25), (v >> 26) & 0xFFF, ((v & 0x3FFFFFF) ^ (1 << 25)) - (1 << 25), rvarint(b, 8)[0]))
check('1.8 dig restores the grass block', (12, 59, 7, 32) in changes, changes)
check('1.8 place clears the placed block', (12, 60, 7, 0) in changes, changes)
c.close()

d = Bedrock('ProtectBedrock'); d.join(); time.sleep(1.5)
settings = d.packets(188)
check('bedrock world marked immutable', settings == [b'\x00\x00\x01\x01\x01'], [s.hex() for s in settings])
d.close()
print('failures:', fails)
