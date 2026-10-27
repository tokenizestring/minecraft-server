import json, os, struct, sys, zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
JAR = sys.argv[1] if len(sys.argv) > 1 else os.path.expandvars(r'%APPDATA%\.minecraft\versions\26.3\26.3.jar')
REPORT = os.path.join(HERE, '..', 'reference', 'gen_26_3', 'reports', 'blocks.json')
OUT = os.path.join(HERE, '..', '..', 'data', 'java', 'instant.txt')
BLOCKS = 'net/minecraft/world/level/block/Blocks.class'
PROPS = 'BlockBehaviour$Properties;'
CONSTRUCTED = ('minecraft:infested_',)
SKIPPED = ('minecraft:air', 'minecraft:cave_air', 'minecraft:void_air', 'minecraft:bubble_column')

def parse_pool(data):
    count = struct.unpack('>H', data[8:10])[0]
    pool, pos, index = [None], 10, 1
    while index < count:
        tag = data[pos]
        if tag == 1:
            size = struct.unpack('>H', data[pos + 1:pos + 3])[0]
            pool.append(('utf', data[pos + 3:pos + 3 + size].decode('utf8', 'replace')))
            pos += 3 + size
        elif tag == 4:
            pool.append(('float', struct.unpack('>f', data[pos + 1:pos + 5])[0]))
            pos += 5
        elif tag == 3:
            pool.append(('int', struct.unpack('>i', data[pos + 1:pos + 5])[0]))
            pos += 5
        elif tag in (5, 6):
            pool.extend([('wide',), None])
            pos += 9
            index += 1
        elif tag in (7, 8, 16, 19, 20):
            pool.append((tag, struct.unpack('>H', data[pos + 1:pos + 3])[0]))
            pos += 3
        elif tag in (9, 10, 11, 12, 17, 18):
            pool.append((tag,) + struct.unpack('>HH', data[pos + 1:pos + 5]))
            pos += 5
        elif tag == 15:
            pool.append((tag,))
            pos += 4
        index += 1
    return pool, pos

def member(pool, index):
    entry = pool[index]
    owner = pool[pool[entry[1]][1]][1].split('/')[-1]
    pair = pool[entry[2]]
    return owner, pool[pair[1]][1], pool[pair[2]][1]

def read_methods(data, pool, pos):
    pos += 6
    pos += 2 + 2 * struct.unpack('>H', data[pos:pos + 2])[0]
    for section in range(2):
        count = struct.unpack('>H', data[pos:pos + 2])[0]
        pos += 2
        found = []
        for _ in range(count):
            _, name, desc, attrs = struct.unpack('>HHHH', data[pos:pos + 8])
            pos += 8
            code = None
            for _ in range(attrs):
                attr, size = struct.unpack('>HI', data[pos:pos + 6])
                code = data[pos + 6:pos + 6 + size] if pool[attr][1] == 'Code' else code
                pos += 6 + size
            found.append((pool[name][1], pool[desc][1], code))
    return found

SKIP2 = {0x10, 0x15, 0x16, 0x17, 0x18, 0x19, 0x36, 0x37, 0x38, 0x39, 0x3a, 0xbc, 0xa9}
SKIP3 = {0x11, 0x13, 0x14, 0x84, 0xbb, 0xbd, 0xc0, 0xc1, 0xc6, 0xc7} | set(range(0x99, 0xa9))
FCONST = {0x0b: 0.0, 0x0c: 1.0, 0x0d: 2.0}

def ops(pool, code):
    size = struct.unpack('>I', code[4:8])[0]
    body, at, out = code[8:8 + size], 0, []
    while at < len(body):
        op = body[at]
        if op in (0xb2, 0xb3, 0xb6, 0xb7, 0xb8):
            out.append((op,) + member(pool, struct.unpack('>H', body[at + 1:at + 3])[0]))
            at += 3
        elif op == 0xb9 or op == 0xba:
            at += 5
        elif op in FCONST:
            out.append(('float', FCONST[op]))
            at += 1
        elif op in (0x12, 0x13):
            index = body[at + 1] if op == 0x12 else struct.unpack('>H', body[at + 1:at + 3])[0]
            entry = pool[index]
            out.append(('float', entry[1]) if entry and entry[0] == 'float' else ('const',))
            at += 2 if op == 0x12 else 3
        elif op in SKIP2:
            at += 2
        elif op in SKIP3:
            at += 3
        elif op == 0xc5:
            at += 4
        elif op in (0xc8, 0xc9):
            at += 5
        elif op in (0xaa, 0xab):
            pad = (4 - (at + 1) % 4) % 4
            base = at + 1 + pad
            if op == 0xaa:
                low, high = struct.unpack('>ii', body[base + 4:base + 12])
                at = base + 12 + 4 * (high - low + 1)
            else:
                pairs = struct.unpack('>i', body[base + 4:base + 8])[0]
                at = base + 8 + 8 * pairs
        else:
            out.append(('op', op))
            at += 1
    return out

def strength(seq, at, args):
    floats = [entry[1] for entry in seq[max(0, at - args):at] if entry[0] == 'float']
    return floats[0] == 0.0 if len(floats) == args else None

def walk(seq, helpers, known):
    instant, copied, ident, results = None, None, None, []
    for at, entry in enumerate(seq):
        if entry[0] == 0xb2 and entry[1] in ('BlockItemIds', 'BlockIds') and ident is None:
            ident = entry[2]
        elif entry[0] == 0xb2 and entry[1] == 'Blocks':
            copied = entry[2]
        elif entry[0] == 0xb6 and entry[2] == 'instabreak':
            instant = True
        elif entry[0] == 0xb6 and entry[2] == 'strength':
            found = strength(seq, at, 2 if entry[3].startswith('(FF)') else 1)
            instant = found if found is not None else instant
        elif entry[0] == 0xb8 and entry[2] in ('ofFullCopy', 'ofLegacyCopy'):
            instant = known.get(copied, False)
        elif entry[0] == 0xb8 and entry[1] == 'Blocks' and entry[3].endswith(PROPS):
            instant = helpers.get(entry[2], instant)
        elif entry[0] == 0xb8 and entry[2] == 'of':
            instant = True
        elif entry[0] == 0xb3 and entry[1] == 'Blocks':
            known[entry[2]] = bool(instant)
            results.append(([entry[2]] + ([ident] if ident else []), bool(instant)))
            instant, ident = None, None
    return results, instant

data = zipfile.ZipFile(JAR).read(BLOCKS)
pool, pos = parse_pool(data)
methods = read_methods(data, pool, pos)
helpers, known = {}, {}

for name, desc, code in methods:
    if code and desc.endswith(PROPS) and name != '<clinit>':
        _, tail = walk(ops(pool, code), {}, {})
        helpers[name] = bool(tail)

clinit = next(code for name, desc, code in methods if name == '<clinit>')
results, _ = walk(ops(pool, clinit), helpers, known)
report = json.load(open(REPORT))
picked = [next(('minecraft:' + field.lower() for field in fields if 'minecraft:' + field.lower() in report), None) for fields, instant in results if instant]
missing = [fields for (fields, instant) in results if instant and next((1 for field in fields if 'minecraft:' + field.lower() in report), None) is None]
names = sorted(name for name in picked if name and name not in SKIPPED and name.startswith(CONSTRUCTED) == False)

open(OUT, 'w', newline='\n').write('\n'.join(names) + '\n')
print(len(results), 'blocks read,', len(names), 'break instantly,', len(missing), 'unknown:', ' '.join(missing))
