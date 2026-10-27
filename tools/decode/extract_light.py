import json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'convert'))
from schem_to_world import light_class

REPORT = os.path.join(HERE, '..', 'reference', 'gen_26_3', 'reports', 'blocks.json')
OUT = os.path.join(HERE, '..', '..', 'data', 'java', 'light.txt')

FIXED = {
    'glowstone': 15, 'sea_lantern': 15, 'jack_o_lantern': 15, 'lantern': 15, 'shroomlight': 15, 'beacon': 15, 'conduit': 15,
    'end_gateway': 15, 'end_portal': 15, 'fire': 15, 'lava': 15, 'ochre_froglight': 15, 'verdant_froglight': 15, 'pearlescent_froglight': 15,
    'torch': 14, 'wall_torch': 14, 'copper_torch': 14, 'copper_wall_torch': 14, 'end_rod': 14,
    'nether_portal': 11, 'crying_obsidian': 10, 'soul_torch': 10, 'soul_wall_torch': 10, 'soul_lantern': 10, 'soul_fire': 10,
    'glow_lichen': 7, 'enchanting_table': 7, 'ender_chest': 7, 'sculk_catalyst': 6, 'amethyst_cluster': 5, 'large_amethyst_bud': 4,
    'magma_block': 3, 'medium_amethyst_bud': 2, 'firefly_bush': 2, 'small_amethyst_bud': 1, 'brewing_stand': 1, 'brown_mushroom': 1,
    'dragon_egg': 1, 'end_portal_frame': 1, 'sculk_sensor': 1, 'calibrated_sculk_sensor': 1,
}
LIT = {'campfire': 15, 'redstone_lamp': 15, 'soul_campfire': 10, 'furnace': 13, 'blast_furnace': 13, 'smoker': 13, 'redstone_ore': 9, 'deepslate_redstone_ore': 9, 'redstone_torch': 7, 'redstone_wall_torch': 7, 'candle_cake': 3}
BULBS = {'': 15, 'exposed_': 12, 'weathered_': 8, 'oxidized_': 4}
ANCHOR = [0, 3, 7, 11, 15]

def emission(short, props):
    lit = props.get('lit') == 'true'
    level = FIXED.get(short, 0)
    level = LIT[short] if short in LIT and lit else level
    level = 15 if 'copper_lantern' in short else level
    level = BULBS.get(short.replace('waxed_', '').replace('copper_bulb', ''), 0) if short.endswith('copper_bulb') and lit else level
    level = 3 * int(props.get('candles', 1)) if short.endswith('candle') and lit else level
    level = 3 + 3 * int(props.get('pickles', 1)) if short == 'sea_pickle' and props.get('waterlogged') == 'true' else level
    level = 14 if short in ('cave_vines', 'cave_vines_plant') and props.get('berries') == 'true' else level
    level = ANCHOR[int(props.get('charges', 0))] if short == 'respawn_anchor' else level
    level = int(props.get('level', 0)) if short == 'light' else level
    return level

rows = []
for name, block in json.load(open(REPORT)).items():
    short = name.split(':', 1)[1]
    for state in block['states']:
        props = state.get('properties', {})
        level = emission(short, props)
        opacity = {0: 0, 8: 1, 4: 15}[light_class(name, props) & 12] if name not in ('minecraft:air', 'minecraft:cave_air', 'minecraft:void_air') else 0
        if level or opacity != 15:
            rows.append('%d %d %d' % (state['id'], level, opacity))

open(OUT, 'w', newline='\n').write('\n'.join(rows) + '\n')
print(len(rows), 'states with light or see-through,', sum(1 for r in rows if r.split()[1] != '0'), 'emit')
