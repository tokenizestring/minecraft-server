import time, sys, os, struct
sys.path[:0] = [os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', d) for d in ('probes', 'decode')]
from clients import *

BOTS = int(sys.argv[1]) if len(sys.argv) > 1 else 30
SECONDS = float(sys.argv[2]) if len(sys.argv) > 2 else 75.0
SPREAD = 160

cb3, sb3 = packet_ids('26_3')
bots = []
for index in range(BOTS):
    bot = Java('MobLoad%d' % index, 777); bot.join(); bot.send(sb3['chat_command'], mstr('smp')); bots.append(bot); time.sleep(0.15)
time.sleep(2.0)
side = int(BOTS ** 0.5) + 1
for index, bot in enumerate(bots):
    x, z = (index % side) * SPREAD, (index // side) * SPREAD
    bot.send(sb3['move_player_pos_rot'], struct.pack('>dddffB', x + 0.5, 140.0, z + 0.5, 0.0, 0.0, 0))
start = time.time()
seen = 0
while time.time() - start < SECONDS:
    time.sleep(1.0)
    for bot in bots:
        with bot.lock:
            pings = [b for p, b in bot.log if p == cb3['keep_alive']]
            bot.log.clear()
        for ping in pings:
            bot.send(sb3['keep_alive'], ping[:8])
total = 0
for bot in bots:
    bot.close()
print('bots', BOTS, 'ran', round(time.time() - start, 1), 's')
