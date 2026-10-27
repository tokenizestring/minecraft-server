import time, sys, os
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *
import bedrock_addplayer, bedrock_playerlist

fails = 0
def check(label, condition, detail=''):
    global fails
    print('PASS' if condition else 'FAIL', label, detail)
    fails += 0 if condition else 1

a = Java('Java263', 777); a.join(); time.sleep(0.5)
b = Java('Java262', 776); b.join(); time.sleep(0.5)
c = Java('Java18', 47); c.join(); time.sleep(0.5)
d = Bedrock('BedrockOne'); d.join(); time.sleep(2.0)

check('26.3 sees 3 players and the npc', len(set(a.entity_ids('add'))) == 4, a.entity_ids('add'))
check('26.2 sees 3 players and the npc', len(set(b.entity_ids('add'))) == 4, b.entity_ids('add'))
check('1.8 sees 3 players and the npc', len(set(c.entity_ids('add'))) == 4, c.entity_ids('add'))
adds = d.packets(12)
check('bedrock sees 3 players and the npc', len(adds) == 4, len(adds))
names = sorted(bedrock_addplayer.parse(x)['name'] for x in adds)
check('bedrock addplayer bytes exact', all(bedrock_addplayer.parse(x)['consumed'].split(' of ')[0] == str(len(x)) for x in adds), names)
lists = [x for x in d.packets(63) if x[2] == 0]
check('bedrock playerlist bytes exact (5 incl self and npc)', len(lists) == 5 and all(bedrock_playerlist.parse(x)['consumed'].split(' of ')[0] == str(len(x)) for x in lists), len(lists))

b_id = [e for e in a.entity_ids('add')]
b.move(8.5, 65.0, 9.5, 90.0); time.sleep(0.6)
moves_a = a.entity_ids('move'); moves_c = c.entity_ids('move'); moves_d = [rvarint(x, 0)[0] for x in d.packets(19)]
check('26.2 move reaches 26.3', len(moves_a) >= 1, moves_a)
check('26.2 move reaches 1.8', len(moves_c) >= 1, moves_c)
check('26.2 move reaches bedrock', len(moves_d) >= 1, moves_d)
check('bedrock moveplayer layout matches BDS (teleport-data flag)', all(move_exact(x) for x in d.packets(19)), [x.hex(' ') for x in d.packets(19)][:1])
d.move(9.5, 65.0, 8.5, 45.0); time.sleep(0.6)
check('bedrock move reaches 26.3', len(set(a.entity_ids('move'))) >= 2, a.entity_ids('move'))
check('bedrock move reaches 1.8', len(set(c.entity_ids('move'))) >= 2, c.entity_ids('move'))

b.close(); time.sleep(1.5)
check('26.3 sees 26.2 removed', len(a.removed_ids()) == 1, a.removed_ids())
check('1.8 sees 26.2 removed', len(c.removed_ids()) == 1, c.removed_ids())
check('bedrock sees 26.2 removed', len(d.packets(14)) == 1, len(d.packets(14)))
d.close(); time.sleep(1.5)
check('26.3 sees bedrock removed', len(a.removed_ids()) == 2, a.removed_ids())
for x in (a, c): x.close()
print('failures:', fails)
