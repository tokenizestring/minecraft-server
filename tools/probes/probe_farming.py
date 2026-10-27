import time, sys, os, struct, json, random, subprocess, threading
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, '..', '..')
BIN = os.path.join(ROOT, 'build', 'bin')
LOG = os.path.join(BIN, 'server.log')
CONFIG = os.path.join(BIN, 'server.txt')

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail, flush=True)
    fails += 0 if condition else 1

def report(tag):
    states = {}
    for name, block in json.load(open(os.path.join(ROOT, 'tools', 'reference', 'gen_' + tag, 'reports', 'blocks.json'))).items():
        for state in block['states']:
            states[state['id']] = (name, state.get('properties', {}))
    return states

states3, states2 = report('26_3'), report('26_2')
first3 = {name: min(i for i, (n, _) in states3.items() if n == name) for name in ('minecraft:wheat', 'minecraft:carrots', 'minecraft:torch')}
first2 = {name: min(i for i, (n, _) in states2.items() if n == name) for name in ('minecraft:wheat', 'minecraft:torch')}
items3 = [line.split()[0] for line in open(os.path.join(ROOT, 'data', 'java', 'items_26_3.txt')) if line.strip()]
cb3, sb3 = packet_ids('26_3')
cb2, sb2 = packet_ids('26_2')
START3, STOP3 = 0, 3

def growth_setting():
    return int(next(line.split('=')[1] for line in open(CONFIG) if line.startswith('crop-growth=')))

def set_growth(value):
    lines = [('crop-growth=%d\n' % value) if line.startswith('crop-growth=') else line for line in open(CONFIG)]
    open(CONFIG, 'w', newline='\n').write(''.join(lines))

def log_size():
    return os.path.getsize(LOG)

def log_since(offset):
    with open(LOG, 'rb') as handle:
        handle.seek(offset)
        return handle.read().decode('utf8', 'replace')

def wait_log(offset, text, seconds):
    end = time.time() + seconds
    while time.time() < end and text not in log_since(offset):
        time.sleep(0.5)
    return text in log_since(offset)

def restart(growth):
    set_growth(growth)
    subprocess.run(['powershell', '-NoProfile', '-Command', 'Stop-Process -Name server -ErrorAction SilentlyContinue'], check=False)
    time.sleep(1.5)
    offset = log_size()
    subprocess.Popen([os.path.join(BIN, 'server.exe')], cwd=BIN, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL, creationflags=0x00000008 | 0x00000200)
    return wait_log(offset, '[drops](load)', 60)

def profile(name, lines):
    open(os.path.join(BIN, 'players', name.lower() + '.txt'), 'w', newline='\n').write('balance 100\nvitals 20.0 20 5.0\n' + ''.join('slot %d minecraft:%s %d\n' % line for line in lines))

def pos(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((z & 0x3FFFFFF) << 12) | (y & 0xFFF))

def pos18(x, y, z):
    return struct.pack('>Q', ((x & 0x3FFFFFF) << 38) | ((y & 0xFFF) << 26) | (z & 0x3FFFFFF))

def bodies(client, pid):
    with client.lock:
        return [b for p, b in client.log if p == pid]

def updates(client, pid, packed):
    return [rvarint(b, 8)[0] for b in bodies(client, pid) if b[:8] == packed]

def use_on(client, sb, spot, sequence):
    client.send(sb['use_item_on'], varint(0) + pos(*spot) + varint(1) + struct.pack('>fff??', 0.5, 1.0, 0.5, False, False) + varint(sequence))

def action(client, sb, status, spot, sequence):
    client.send(sb['player_action'], varint(status) + pos(*spot) + b'\x01' + varint(sequence))

def hold(client, sb, slot):
    client.send(sb['set_carried_item'], struct.pack('>H', slot))

def item_drops(client, cb, kind):
    out = []
    with client.lock:
        added = [rvarint(b, 0)[0] for p, b in client.log if p == cb['add_entity'] and rvarint(b, rvarint(b, 0)[1] + 16)[0] == kind]
        for p, b in client.log:
            if p == cb['set_entity_data'] and rvarint(b, 0)[0] in added:
                q = rvarint(b, 0)[1]
                if b[q:q + 2] == b'\x08\x07':
                    count, q = rvarint(b, q + 2)
                    item, q = rvarint(b, q)
                    out.append((items3[item] if item < len(items3) else item, count))
    return out

def sections_of(body):
    x, z = struct.unpack('>ii', body[:8]); p = 8
    count, p = rvarint(body, p)
    for _ in range(count):
        kind, p = rvarint(body, p); n, p = rvarint(body, p); p += 8 * n
    size, p = rvarint(body, p); out = []
    for _ in range(24):
        p += 4
        bits = body[p]; p += 1
        if bits == 0:
            value, p = rvarint(body, p); blocks = [value] * 4096
        else:
            palette = None
            if bits <= 8:
                n, p = rvarint(body, p); palette = []
                for _ in range(n):
                    value, p = rvarint(body, p); palette.append(value)
            per = 64 // bits; longs = (4096 + per - 1) // per
            words = struct.unpack('>%dQ' % longs, body[p:p + 8 * longs]); p += 8 * longs
            raw = [(words[i // per] >> ((i % per) * bits)) & ((1 << bits) - 1) for i in range(4096)]
            blocks = [palette[v] for v in raw] if palette else raw
        p += 1; _, p = rvarint(body, p)
        out.append(blocks)
    return x, z, out

def world_of(client):
    found = {}
    for body in bodies(client, cb3['level_chunk_with_light']):
        x, z, sections = sections_of(body)
        found[(x, z)] = sections
    return found

def block_at(found, x, y, z):
    return found[(x >> 4, z >> 4)][(y + 64) >> 4][(y & 15) * 256 + (z & 15) * 16 + (x & 15)]

def top_of(found, x, z):
    for y in range(255, -64, -1):
        state = block_at(found, x, y, z)
        if not states3[state][0].endswith(':air'):
            return y, states3[state][0]
    return None, None

def age_of(state):
    return int(states3[state][1].get('age', -1)) if state in states3 else -1

def go_smp(client, command):
    client.send(command[0], mstr(command[1]))

def keep_alive(client):
    cb, sb = ({'keep_alive': 0x00}, {'keep_alive': 0x00}) if client.protocol == 47 else packet_ids('26_3' if client.protocol == 777 else '26_2')
    answered = set()
    def loop():
        while client.running:
            for ping in bodies(client, cb['keep_alive']):
                if ping not in answered:
                    answered.add(ping)
                    try:
                        client.send(sb['keep_alive'], ping)
                    except OSError:
                        return
            time.sleep(1.0)
    threading.Thread(target=loop, daemon=True).start()
    return client

def join_all():
    farmer = keep_alive(Java('Farmhand', 777)); farmer.join(); time.sleep(0.5)
    watch2 = keep_alive(Java('FarmWatch262', 776)); watch2.join(); time.sleep(0.4)
    watch8 = keep_alive(Java('FarmWatch18', 47)); watch8.join(); time.sleep(0.4)
    go_smp(farmer, (sb3['chat_command'], 'smp')); go_smp(watch2, (sb2['chat_command'], 'smp')); go_smp(watch8, (0x01, '/smp'))
    time.sleep(3.0)
    return farmer, watch2, watch8

growth = growth_setting()
if growth != 4:
    check('server runs crop-growth=4 to start with', restart(4))

profile('Farmhand', [(0, 'wooden_hoe', 1), (1, 'wheat_seeds', 16), (2, 'carrot', 16), (3, 'torch', 4)])
profile('FarmWatch262', [(0, 'torch', 4)])
profile('FarmWatch18', [(0, 'torch', 4), (1, 'wooden_hoe', 1), (2, 'wheat_seeds', 4)])
farmer, watch2, watch8 = join_all()

with farmer.lock: spot = [x for p, x in farmer.log if p == cb3['player_position']][-1]
sx, sy, sz = [int(v // 1) for v in teleport_spot(spot)]
found = world_of(farmer)
grass, plants = [], []
for dx in range(-14, 15):
    for dz in range(-14, 15):
        x, z = sx + dx, sz + dz
        if (x >> 4, z >> 4) in found and ((x - 1) >> 4, z >> 4) in found:
            y, name = top_of(found, x, z)
            if name == 'minecraft:grass_block' and abs(dx) > 1 and abs(dz) > 1:
                grass.append((x, y, z))
            if name in ('minecraft:short_grass', 'minecraft:dandelion', 'minecraft:poppy', 'minecraft:fern'):
                plants.append((x, y, z, name))
random.shuffle(grass)
check('found open grass around the smp spawn', len(grass) >= 8, len(grass))
spots = grass[:8]
wheat_soil, carrot_soil, pop_soil, soil18, torch3, torch2, torch8, bare = spots
farmer.move(sx + 0.5, sy, sz + 0.5, 0.0); watch2.move(sx + 0.5, sy, sz + 1.5, 0.0); watch8.move(sx + 1.5, sy, sz + 0.5, 0.0)
time.sleep(0.5)

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
hold(farmer, sb3, 3)
use_on(farmer, sb3, torch3, 10); time.sleep(0.4)
above = (torch3[0], torch3[1] + 1, torch3[2])
check('26.3 survival places a torch', updates(farmer, cb3['block_update'], pos(*above)) == [first3['minecraft:torch']], updates(farmer, cb3['block_update'], pos(*above)))
action(farmer, sb3, START3, above, 11); time.sleep(0.6)
check('26.3 survival breaks the torch with only START', updates(farmer, cb3['block_update'], pos(*above))[-1:] == [0], updates(farmer, cb3['block_update'], pos(*above)))
check('26.2 and 1.8 see the torch go', updates(watch2, cb2['block_update'], pos(*above))[-1:] == [0] and updates(watch8, 0x23, pos18(*above))[-1:] == [0])
check('the torch drops as an item', ('minecraft:torch', 1) in item_drops(farmer, cb3, 72), item_drops(farmer, cb3, 72))

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
hold(watch2, sb2, 0)
above2 = (torch2[0], torch2[1] + 1, torch2[2])
use_on(watch2, sb2, torch2, 12); time.sleep(0.4)
check('26.2 survival places a torch', updates(watch2, cb2['block_update'], pos(*above2)) == [first2['minecraft:torch']], updates(watch2, cb2['block_update'], pos(*above2)))
action(watch2, sb2, 0, above2, 13); time.sleep(0.6)
check('26.2 survival breaks the torch with only START', updates(watch2, cb2['block_update'], pos(*above2))[-1:] == [0], updates(watch2, cb2['block_update'], pos(*above2)))
check('and it drops for 26.3 to see', ('minecraft:torch', 1) in item_drops(farmer, cb3, 72), item_drops(farmer, cb3, 72))

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
watch8.send(0x09, struct.pack('>H', 0))
above8 = (torch8[0], torch8[1] + 1, torch8[2])
watch8.send(0x08, pos18(*torch8) + b'\x01' + struct.pack('>hbhb', 50, 1, 0, 0) + b'\x08\x10\x08'); time.sleep(0.4)
check('1.8 survival places a torch', updates(farmer, cb3['block_update'], pos(*above8)) == [first3['minecraft:torch']], updates(farmer, cb3['block_update'], pos(*above8)))
watch8.send(0x07, b'\x00' + pos18(*above8) + b'\x01'); time.sleep(0.6)
check('1.8 survival breaks the torch with only START', updates(farmer, cb3['block_update'], pos(*above8))[-1:] == [0] and updates(watch8, 0x23, pos18(*above8))[-1:] == [0], updates(watch8, 0x23, pos18(*above8)))
check('and it drops for 26.3 to see', ('minecraft:torch', 1) in item_drops(farmer, cb3, 72), item_drops(farmer, cb3, 72))

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
action(farmer, sb3, START3, bare, 14); time.sleep(0.4)
check('START alone does not break grass_block in survival', updates(farmer, cb3['block_update'], pos(*bare)) == [], updates(farmer, cb3['block_update'], pos(*bare)))
if plants:
    plant = plants[0]
    action(farmer, sb3, START3, plant[:3], 15); time.sleep(0.4)
    check('START alone breaks generated %s' % plant[3], updates(farmer, cb3['block_update'], pos(*plant[:3]))[-1:] == [0], updates(farmer, cb3['block_update'], pos(*plant[:3])))

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
hold(farmer, sb3, 1)
use_on(farmer, sb3, bare, 16); time.sleep(0.4)
bare_up = (bare[0], bare[1] + 1, bare[2])
check('seeds do not plant on plain grass', first3['minecraft:wheat'] not in updates(farmer, cb3['block_update'], pos(*bare_up)), updates(farmer, cb3['block_update'], pos(*bare_up)))
hold(farmer, sb3, 0)
for index, soil in enumerate((wheat_soil, carrot_soil, pop_soil)):
    use_on(farmer, sb3, soil, 20 + index)
time.sleep(0.5)
tilled = [updates(farmer, cb3['block_update'], pos(*soil)) for soil in (wheat_soil, carrot_soil, pop_soil)]
check('the hoe tills grass into farmland', all(u and states3[u[-1]][0] == 'minecraft:farmland' for u in tilled), tilled)
check('26.2 and 1.8 see the farmland', updates(watch2, cb2['block_update'], pos(*wheat_soil)) and (updates(watch8, 0x23, pos18(*wheat_soil))[-1:] or [0])[0] >> 4 == 60)

wheat = (wheat_soil[0], wheat_soil[1] + 1, wheat_soil[2])
carrot = (carrot_soil[0], carrot_soil[1] + 1, carrot_soil[2])
hold(farmer, sb3, 1)
plant_offset = log_size()
use_on(farmer, sb3, wheat_soil, 30)
planted = time.time()
hold(farmer, sb3, 2)
use_on(farmer, sb3, carrot_soil, 31); time.sleep(0.5)
check('wheat seeds plant wheat at age 0', updates(farmer, cb3['block_update'], pos(*wheat)) == [first3['minecraft:wheat']], updates(farmer, cb3['block_update'], pos(*wheat)))
check('26.2 sees wheat age 0 and 1.8 sees block 59', updates(watch2, cb2['block_update'], pos(*wheat)) == [first2['minecraft:wheat']] and updates(watch8, 0x23, pos18(*wheat)) == [59 << 4], (updates(watch2, cb2['block_update'], pos(*wheat)), updates(watch8, 0x23, pos18(*wheat))))
check('a carrot plants carrots', updates(farmer, cb3['block_update'], pos(*carrot)) == [first3['minecraft:carrots']], updates(farmer, cb3['block_update'], pos(*carrot)))

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
action(farmer, sb3, START3, carrot, 32); time.sleep(0.8)
check('a young carrot breaks on START and drops one carrot', updates(farmer, cb3['block_update'], pos(*carrot))[-1:] == [0] and item_drops(farmer, cb3, 72) == [('minecraft:carrot', 1)], item_drops(farmer, cb3, 72))

watch8.send(0x09, struct.pack('>H', 1))
watch8.send(0x08, pos18(*soil18) + b'\x01' + struct.pack('>hbhb', 290, 1, 0, 0) + b'\x08\x10\x08'); time.sleep(0.4)
watch8.send(0x09, struct.pack('>H', 2))
watch8.send(0x08, pos18(*soil18) + b'\x01' + struct.pack('>hbhb', 295, 1, 0, 0) + b'\x08\x10\x08'); time.sleep(0.5)
up18 = (soil18[0], soil18[1] + 1, soil18[2])
check('1.8 tills and plants wheat that 26.3 sees', states3[(updates(farmer, cb3['block_update'], pos(*soil18)) or [0])[-1]][0] == 'minecraft:farmland' and updates(farmer, cb3['block_update'], pos(*up18)) == [first3['minecraft:wheat']], (updates(farmer, cb3['block_update'], pos(*soil18)), updates(farmer, cb3['block_update'], pos(*up18))))

stage_seconds = 4096 / 20 / 4
grown = None
while time.time() - planted < stage_seconds + 8 and grown is None:
    time.sleep(0.25)
    if first3['minecraft:wheat'] + 1 in updates(farmer, cb3['block_update'], pos(*wheat)):
        grown = time.time() - planted
check('wheat reaches age 1 after one 4x stage (%.1f s)' % stage_seconds, grown is not None and abs(grown - stage_seconds) <= 2.0, grown)
check('which is 4x the vanilla 204.8 s per stage', grown is not None and abs(204.8 / grown - 4.0) < 0.2, grown and 204.8 / grown)
check('26.2 and 1.8 watchers get the stage too', first2['minecraft:wheat'] + 1 in updates(watch2, cb2['block_update'], pos(*wheat)) and (59 << 4) | 1 in updates(watch8, 0x23, pos18(*wheat)))

saved = log_size()
check('the growing crop is saved by the autosave', wait_log(plant_offset, '[edits](save)', 75))
for client in (farmer, watch2, watch8): client.close()
time.sleep(0.5)
check('server restarts with crop-growth=4', restart(4))
check('the save loads back with crops growing', 'crops growing' in log_since(saved) and ' 0 crops growing' not in log_since(saved), [line for line in log_since(saved).splitlines() if 'crops growing' in line])

checker = keep_alive(Java('FarmCheck', 777)); checker.join(); time.sleep(0.4)
go_smp(checker, (sb3['chat_command'], 'smp')); time.sleep(3.0)
checker.move(sx + 0.5, sy, sz + 0.5, 0.0); time.sleep(0.5)
seen = world_of(checker)
expected = int((time.time() - planted) * 20 * 4 // 4096)
age = age_of(block_at(seen, *wheat))
check('after a restart the chunk shows the wheat at its real age', abs(age - expected) <= 1 and age >= 1, (age, expected))
with checker.lock: checker.log.clear()
deadline = time.time() + stage_seconds + 6
nxt = None
while time.time() < deadline and nxt is None:
    time.sleep(0.5)
    ups = [age_of(state) for state in updates(checker, cb3['block_update'], pos(*wheat))]
    nxt = ups[-1] if ups else None
check('and the reloaded crop keeps growing (next stage pushed)', nxt is not None and nxt > age, (age, nxt))
checker.close()

check('server restarts with crop-growth=400', restart(400))
farmer, watch2, watch8 = join_all()
farmer.move(sx + 0.5, sy, sz + 0.5, 0.0); time.sleep(0.6)
seen = world_of(farmer)
check('at 400x the old wheat loads fully grown', age_of(block_at(seen, *wheat)) == 7, age_of(block_at(seen, *wheat)))
for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
action(farmer, sb3, START3, wheat, 40); time.sleep(1.0)
drops = item_drops(farmer, cb3, 72)
check('harvesting grown wheat breaks it on START', updates(farmer, cb3['block_update'], pos(*wheat))[-1:] == [0])
check('grown wheat drops wheat and 1-4 seeds', ('minecraft:wheat', 1) in drops and any(name == 'minecraft:wheat_seeds' and 1 <= count <= 4 for name, count in drops), drops)

for client in (farmer, watch2, watch8):
    with client.lock: client.log.clear()
hold(farmer, sb3, 1)
popped = (pop_soil[0], pop_soil[1] + 1, pop_soil[2])
use_on(farmer, sb3, pop_soil, 41)
time.sleep(5.0)
ages = [age_of(state) for state in updates(farmer, cb3['block_update'], pos(*popped))]
check('a new crop climbs through its stages at 400x', ages[:1] == [0] and ages[-1:] == [7] and ages == sorted(ages), ages)
check('26.2 sees the final stage', updates(watch2, cb2['block_update'], pos(*popped))[-1:] == [first2['minecraft:wheat'] + 7], updates(watch2, cb2['block_update'], pos(*popped)))
with farmer.lock: farmer.log.clear()
action(farmer, sb3, START3, pop_soil, 42); action(farmer, sb3, STOP3, pop_soil, 43); time.sleep(1.0)
drops = item_drops(farmer, cb3, 72)
check('breaking the farmland pops the crop on top', updates(farmer, cb3['block_update'], pos(*popped))[-1:] == [0] and ('minecraft:wheat', 1) in drops, (updates(farmer, cb3['block_update'], pos(*popped)), drops))
check('and the farmland drops dirt', ('minecraft:dirt', 1) in drops, drops)

for client in (farmer, watch2, watch8): client.close()
saved = log_size()
wait_log(saved, '[edits](save)', 75)
check('server goes back to crop-growth=4', restart(4))
print('failures:', fails)
