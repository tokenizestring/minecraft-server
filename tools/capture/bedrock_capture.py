import os, sys, json, time, struct, zlib, hashlib, base64, uuid
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
import bedrock_probe as probe
import bedrock_crypto as crypto

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 19133
TAG = sys.argv[2] if len(sys.argv) > 2 else 'bds'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'captures', 'capture_bedrock_' + TAG)
probe.PORT = PORT

class Session(probe.Client):
    def __init__(self):
        super().__init__(1400)
        self.encrypt = None
        self.decrypt = None
        self.key = None
        self.send_counter = 0
        self.recv_counter = 0
        self.zlib_out = True
        self.log = []

    def game(self, packets, compress=False):
        batch = b''.join(probe.uvarint(len(p)) + p for p in packets)
        if self.compression:
            c = zlib.compressobj(6, zlib.DEFLATED, -15)
            batch = b'\x00' + c.compress(batch) + c.flush()
        if self.encrypt:
            checksum = hashlib.sha256(struct.pack('<Q', self.send_counter) + batch + self.key).digest()[:8]
            self.send_counter += 1
            batch = self.encrypt.process(batch + checksum)
        self.send_frames(b'\xfe' + batch)

    def decode_game(self, msg):
        body = msg[1:]
        if self.decrypt:
            body = self.decrypt.process(body)[:-8]
            self.recv_counter += 1
        if self.compression:
            algorithm, body = body[0], body[1:]
            if algorithm == 0x00:
                body = zlib.decompress(body, -15)
            elif algorithm != 0xFF:
                raise RuntimeError('compression %x' % algorithm)
        out, p = [], 0
        while p < len(body):
            n, p = probe.ruvarint(body, p)
            pkt = body[p:p + n]; p += n
            header, q = probe.ruvarint(pkt, 0)
            out.append((header & 0x3FF, pkt[q:]))
        return out

    def pump(self, seconds):
        got = []
        for m in self.messages(seconds):
            if m[0] == 0xFE:
                for pid, body in self.decode_game(m):
                    self.log.append((pid, body))
                    got.append((pid, body))
        return got

def skin_data():
    geometry = base64.b64encode(json.dumps({'format_version': '1.12.0', 'minecraft:geometry': []}).encode()).decode()
    return {
        'AnimatedImageData': [], 'ArmSize': 'wide', 'CapeData': '', 'CapeId': '', 'CapeImageHeight': 0, 'CapeImageWidth': 0,
        'CapeOnClassicSkin': False, 'ClientRandomId': 1234567, 'CompatibleWithClientSideChunkGen': False, 'CurrentInputMode': 1,
        'DefaultInputMode': 1, 'DeviceId': str(uuid.uuid4()), 'DeviceModel': 'probe', 'DeviceOS': 7, 'GameVersion': '1.26.52',
        'GuiScale': 0, 'IsEditorMode': False, 'LanguageCode': 'en_US', 'MaxViewDistance': 16, 'MemoryTier': 4, 'OverrideSkin': False,
        'PersonaPieces': [], 'PersonaSkin': False, 'PieceTintColors': [], 'PlatformOfflineId': '', 'PlatformOnlineId': '',
        'PlatformType': 0, 'PlayFabId': '0000000000000000', 'PremiumSkin': False, 'SelfSignedId': str(uuid.uuid4()),
        'ServerAddress': '127.0.0.1:%d' % PORT, 'SkinAnimationData': '', 'SkinColor': '#0', 'SkinData': base64.b64encode(b'\xff' * 64 * 64 * 4).decode(),
        'SkinGeometryData': geometry, 'SkinGeometryDataEngineVersion': base64.b64encode(b'1.26.52').decode(), 'SkinId': 'Standard_Custom',
        'SkinImageHeight': 64, 'SkinImageWidth': 64,
        'SkinResourcePatch': base64.b64encode(json.dumps({'geometry': {'default': 'geometry.humanoid.custom'}}).encode()).decode(),
        'ThirdPartyName': 'Probe', 'TrustedSkin': False, 'UIProfile': 0,
        'ClientEditorConnectionIntent': 0, 'ClientIsEditorCapable': False, 'FilterProfanity': False, 'GraphicsMode': 1,
        'ProfileHash': base64.b64encode(os.urandom(32)).decode(),
    }

def main():
    os.makedirs(OUT, exist_ok=True)
    s = Session()
    probe.connect(s)
    s.game([probe.uvarint(193) + struct.pack('>i', probe.PROTOCOL)])
    got = s.pump(1)
    assert got and got[0][0] == 143, got
    print('network settings', got[0][1].hex())
    s.compression = True

    key = crypto.Key()
    now = int(time.time())
    identity = crypto.jwt(key, {'certificateAuthority': True, 'exp': now + 86400, 'identityPublicKey': key.public_der_b64(), 'nbf': now - 60, 'extraData': {'displayName': 'Probe', 'identity': str(uuid.uuid4()), 'XUID': '', 'titleId': '896928775'}})
    token = crypto.jwt(key, {'sub': 'probe', 'iat': now, 'exp': now + 86400, 'cpk': key.public_der_b64(), 'xid': os.environ.get('PROBE_XID', ''), 'xname': os.environ.get('PROBE_NAME', 'Probe')})
    style = os.environ.get('LOGIN_STYLE', 'certificate')
    auth = int(os.environ.get('AUTH_TYPE', '2'))
    if style == 'legacy':
        chain = json.dumps({'chain': [identity]})
    elif style == 'token':
        chain = json.dumps({'AuthenticationType': auth, 'Token': token})
    elif style == 'both':
        chain = json.dumps({'AuthenticationType': auth, 'Certificate': json.dumps({'chain': [identity]}), 'Token': token})
    else:
        chain = json.dumps({'AuthenticationType': auth, 'Certificate': json.dumps({'chain': [identity]}), 'Token': ''})
    client = crypto.jwt(key, skin_data())
    if os.environ.get('CLIENT_FROM'):
        raw = open(os.environ['CLIENT_FROM'], 'rb').read()[8:]
        first = struct.unpack('<I', raw[:4])[0]
        real = crypto.jwt_payload(raw[8 + first:].decode())
        real['ServerAddress'] = '127.0.0.1:%d' % PORT
        real['ThirdPartyName'] = os.environ.get('PROBE_NAME', real.get('ThirdPartyName'))
        real['SelfSignedId'] = str(uuid.uuid4())
        client = crypto.jwt(key, real)
    request = struct.pack('<I', len(chain)) + chain.encode() + struct.pack('<I', len(client)) + client.encode()
    login = probe.uvarint(1) + struct.pack('>i', probe.PROTOCOL) + probe.uvarint(len(request)) + request
    if os.environ.get('LOGIN_REPLAY'):
        login = open(os.environ['LOGIN_REPLAY'], 'rb').read()
    print('login packet', len(login), login[:12].hex(' '))
    s.game([login])

    deadline = time.time() + 20
    stage = 'login'
    while time.time() < deadline:
        for pid, body in s.pump(0.5):
            print('<-', pid, len(body))
            if pid == 3:
                token = body[probe.ruvarint(body, 0)[1]:].decode()
                server_key = crypto.jwt_header(token)['x5u']
                salt = base64.b64decode(crypto.jwt_payload(token)['salt'])
                s.key = hashlib.sha256(salt + key.shared_secret(server_key)).digest()
                iv = s.key[:12] + b'\x00\x00\x00\x02'
                s.encrypt = crypto.CTR(s.key, iv)
                s.decrypt = crypto.CTR(s.key, iv)
                s.game([probe.uvarint(4)])
                print('   encryption enabled')
            elif pid == 5:
                print('   disconnect', body)
                deadline = 0
            elif pid == 6:
                s.game([probe.uvarint(8) + probe.uvarint(2) + probe.uvarint(19) + b'downloadingfinished'])
            elif pid == 7:
                s.game([probe.uvarint(8) + probe.uvarint(3) + probe.uvarint(25) + b'resourcepackstackfinished'])
            elif pid == 11:
                s.game([probe.uvarint(69) + probe.uvarint(8) + b'\x08'])
                stage = 'started'
            elif pid == 2 and struct.unpack('>i', body)[0] == 3:
                s.game([probe.uvarint(113) + probe.uvarint(1)])
                stage = 'spawned'
                deadline = min(deadline, time.time() + float(os.environ.get('STAY', '2')))

    if os.environ.get('SUBCHUNK'):
        chunk = next(body for pid, body in s.log if pid == 58)
        cx, q = probe.ruvarint(chunk, 0); cz, q = probe.ruvarint(chunk, q)
        cx = (cx >> 1) ^ -(cx & 1); cz = (cz >> 1) ^ -(cz & 1)
        if os.environ.get('SUBCHUNK_CHUNK'):
            cx, cz = map(int, os.environ['SUBCHUNK_CHUNK'].split(','))
        offsets = b''.join(struct.pack('bbb', 0, dy, 0) for dy in range(-4, 20))
        style = os.environ.get('SUBCHUNK')
        count = struct.pack('<I', 24) if style in ('fixed', 'fixedvar') else probe.uvarint(24)
        center = struct.pack('<iii', cx, 0, cz) if style in ('fixed', 'varfixed') else probe.uvarint((cx << 1) ^ (cx >> 31)) + probe.uvarint(0) + probe.uvarint((cz << 1) ^ (cz >> 31))
        print('requesting subchunks of chunk', cx, cz, 'style', style)
        s.game([probe.uvarint(175) + probe.uvarint(0) + count + offsets + center])
        for pid, body in s.pump(3):
            print('<-', pid, len(body), body[:60].hex(' ') if pid in (174, 156) else '')
            if pid == 156: print('   ', body)
    counts = {}
    for index, (pid, body) in enumerate(s.log):
        counts[pid] = counts.get(pid, 0) + 1
        if counts[pid] <= 3 or pid in (12, 14, 19, 63, 111, 18, 106, 107, 108):
            open(os.path.join(OUT, '%03d_%d.bin' % (index, pid)), 'wb').write(body)
    print('stage', stage, 'packets', counts)

main()
