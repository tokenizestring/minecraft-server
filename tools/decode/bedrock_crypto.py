import hashlib, os, base64, json, time

P = 0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffeffffffff0000000000000000ffffffff
A = P - 3
B = 0xb3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef
N = 0xffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973
G = (0xaa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7,
     0x3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f)
SPKI_PREFIX = bytes.fromhex('3076301006072a8648ce3d020106052b81040022036200')

def point_add(p1, p2):
    if p1 is None: return p2
    if p2 is None: return p1
    if p1[0] == p2[0] and (p1[1] + p2[1]) % P == 0: return None
    if p1 == p2:
        lam = (3 * p1[0] * p1[0] + A) * pow(2 * p1[1], -1, P) % P
    else:
        lam = (p2[1] - p1[1]) * pow(p2[0] - p1[0], -1, P) % P
    x = (lam * lam - p1[0] - p2[0]) % P
    return (x, (lam * (p1[0] - x) - p1[1]) % P)

def scalar_mult(k, point):
    result = None
    while k:
        if k & 1: result = point_add(result, point)
        point = point_add(point, point)
        k >>= 1
    return result

class Key:
    def __init__(self):
        self.d = int.from_bytes(os.urandom(48), 'big') % (N - 1) + 1
        self.q = scalar_mult(self.d, G)

    def public_der_b64(self):
        return base64.b64encode(SPKI_PREFIX + b'\x04' + self.q[0].to_bytes(48, 'big') + self.q[1].to_bytes(48, 'big')).decode()

    def sign(self, message):
        h = int.from_bytes(hashlib.sha384(message).digest(), 'big')
        while True:
            k = int.from_bytes(os.urandom(48), 'big') % (N - 1) + 1
            r = scalar_mult(k, G)[0] % N
            s = pow(k, -1, N) * (h + r * self.d) % N
            if r and s:
                return r.to_bytes(48, 'big') + s.to_bytes(48, 'big')

    def shared_secret(self, public_der_b64):
        raw = base64.b64decode(public_der_b64)[-96:]
        point = (int.from_bytes(raw[:48], 'big'), int.from_bytes(raw[48:], 'big'))
        return scalar_mult(self.d, point)[0].to_bytes(48, 'big')

def b64url(data):
    return base64.urlsafe_b64encode(data).rstrip(b'=').decode()

def jwt(key, payload):
    head = b64url(json.dumps({'alg': 'ES384', 'x5u': key.public_der_b64()}).encode())
    body = b64url(json.dumps(payload).encode())
    return head + '.' + body + '.' + b64url(key.sign((head + '.' + body).encode()))

def jwt_payload(token):
    part = token.split('.')[1]
    return json.loads(base64.urlsafe_b64decode(part + '=' * (-len(part) % 4)))

def jwt_header(token):
    part = token.split('.')[0]
    return json.loads(base64.urlsafe_b64decode(part + '=' * (-len(part) % 4)))

SBOX = [0] * 256
def _init_sbox():
    p = q = 1
    while True:
        p = p ^ ((p << 1) & 0xFF) ^ (0x1B if p & 0x80 else 0)
        q ^= q << 1; q ^= q << 2; q ^= q << 4; q &= 0xFF
        if q & 0x80: q ^= 0x09
        x = q ^ ((q << 1 | q >> 7) & 0xFF) ^ ((q << 2 | q >> 6) & 0xFF) ^ ((q << 3 | q >> 5) & 0xFF) ^ ((q << 4 | q >> 4) & 0xFF)
        SBOX[p] = (x ^ 0x63) & 0xFF
        if p == 1: break
    SBOX[0] = 0x63
_init_sbox()

def xtime(a):
    return ((a << 1) ^ 0x1B) & 0xFF if a & 0x80 else a << 1

def _mul(a, b):
    r = 0
    while b:
        if b & 1: r ^= a
        a = xtime(a); b >>= 1
    return r

T0 = [(_mul(SBOX[i], 2) << 24) | (SBOX[i] << 16) | (SBOX[i] << 8) | _mul(SBOX[i], 3) for i in range(256)]
T1 = [((t >> 8) | (t << 24)) & 0xFFFFFFFF for t in T0]
T2 = [((t >> 16) | (t << 16)) & 0xFFFFFFFF for t in T0]
T3 = [((t >> 24) | (t << 8)) & 0xFFFFFFFF for t in T0]

class AES256:
    def __init__(self, key):
        words = [list(key[i:i + 4]) for i in range(0, 32, 4)]
        rcon = 1
        for i in range(8, 60):
            t = list(words[i - 1])
            if i % 8 == 0:
                t = [SBOX[b] for b in t[1:] + t[:1]]
                t[0] ^= rcon; rcon = xtime(rcon)
            elif i % 8 == 4:
                t = [SBOX[b] for b in t]
            words.append([words[i - 8][j] ^ t[j] for j in range(4)])
        self.round_keys = [sum(words[r * 4:r * 4 + 4], []) for r in range(15)]
        self.round_words = [[int.from_bytes(bytes(words[r * 4 + c]), 'big') for c in range(4)] for r in range(15)]

    def encrypt_block(self, block):
        k = self.round_words
        s0, s1, s2, s3 = (int.from_bytes(block[i:i + 4], 'big') ^ k[0][i // 4] for i in range(0, 16, 4))
        for r in range(1, 14):
            kr = k[r]
            t0 = T0[s0 >> 24] ^ T1[(s1 >> 16) & 255] ^ T2[(s2 >> 8) & 255] ^ T3[s3 & 255] ^ kr[0]
            t1 = T0[s1 >> 24] ^ T1[(s2 >> 16) & 255] ^ T2[(s3 >> 8) & 255] ^ T3[s0 & 255] ^ kr[1]
            t2 = T0[s2 >> 24] ^ T1[(s3 >> 16) & 255] ^ T2[(s0 >> 8) & 255] ^ T3[s1 & 255] ^ kr[2]
            t3 = T0[s3 >> 24] ^ T1[(s0 >> 16) & 255] ^ T2[(s1 >> 8) & 255] ^ T3[s2 & 255] ^ kr[3]
            s0, s1, s2, s3 = t0, t1, t2, t3
        kr = k[14]
        out = b''
        for a, b2, c, d, kw in ((s0, s1, s2, s3, kr[0]), (s1, s2, s3, s0, kr[1]), (s2, s3, s0, s1, kr[2]), (s3, s0, s1, s2, kr[3])):
            w = (SBOX[a >> 24] << 24) | (SBOX[(b2 >> 16) & 255] << 16) | (SBOX[(c >> 8) & 255] << 8) | SBOX[d & 255]
            out += (w ^ kw).to_bytes(4, 'big')
        return out

    def encrypt_block_slow(self, block):
        s = [b ^ k for b, k in zip(block, self.round_keys[0])]
        for r in range(1, 15):
            s = [SBOX[b] for b in s]
            s = [s[(i + 4 * (i % 4)) % 16] for i in range(16)]
            if r != 14:
                out = []
                for c in range(4):
                    a = s[c * 4:c * 4 + 4]
                    t = a[0] ^ a[1] ^ a[2] ^ a[3]
                    out += [a[i] ^ t ^ xtime(a[i] ^ a[(i + 1) % 4]) for i in range(4)]
                s = out
            s = [b ^ k for b, k in zip(s, self.round_keys[r])]
        return bytes(s)

class CTR:
    def __init__(self, key, iv):
        self.aes = AES256(key)
        self.counter = bytearray(iv)
        self.stream = b''

    def _next(self):
        block = self.aes.encrypt_block(bytes(self.counter))
        for i in range(15, -1, -1):
            self.counter[i] = (self.counter[i] + 1) & 0xFF
            if self.counter[i]: break
        return block

    def process(self, data):
        if not data:
            return b''
        blocks = [self.stream]
        have = len(self.stream)
        while have < len(data):
            block = self._next(); blocks.append(block); have += 16
        stream = b''.join(blocks)
        self.stream = stream[len(data):]
        return (int.from_bytes(data, 'big') ^ int.from_bytes(stream[:len(data)], 'big')).to_bytes(len(data), 'big')

def self_test():
    key = bytes(range(32))
    expected = bytes.fromhex('8ea2b7ca516745bfeafc49904b496089')
    assert AES256(key).encrypt_block(bytes.fromhex('00112233445566778899aabbccddeeff')) == expected, 'aes'
    a, b = Key(), Key()
    assert a.shared_secret(b.public_der_b64()) == b.shared_secret(a.public_der_b64()), 'ecdh'
    return 'aes256 fips-197 vector ok, ecdh p-384 ok'

if __name__ == '__main__':
    print(self_test())
