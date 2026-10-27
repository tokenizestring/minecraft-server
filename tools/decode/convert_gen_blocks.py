import json, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'convert'))
import schem_to_world as conv

SOURCE = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\token\Desktop\mcserver\run\data\world\blocks.txt'
OUT = os.path.join(HERE, '..', '..', 'data', 'java', 'gen_blocks.txt')

LEGACY_EXACT = {
    'air': (0, 0), 'stone': (1, 0), 'granite': (1, 1), 'diorite': (1, 3), 'andesite': (1, 5), 'grass_block': (2, 0), 'dirt': (3, 0), 'coarse_dirt': (3, 1),
    'podzol': (3, 2), 'cobblestone': (4, 0), 'oak_planks': (5, 0), 'spruce_planks': (5, 1), 'birch_planks': (5, 2), 'jungle_planks': (5, 3), 'acacia_planks': (5, 4),
    'dark_oak_planks': (5, 5), 'bedrock': (7, 0), 'water': (9, 0), 'lava': (11, 0), 'sand': (12, 0), 'red_sand': (12, 1), 'gravel': (13, 0), 'gold_ore': (14, 0),
    'iron_ore': (15, 0), 'coal_ore': (16, 0), 'oak_log': (17, 0), 'spruce_log': (17, 1), 'birch_log': (17, 2), 'jungle_log': (17, 3), 'oak_wood': (17, 12),
    'stripped_oak_log': (17, 0), 'oak_leaves': (18, 4), 'spruce_leaves': (18, 5), 'birch_leaves': (18, 6), 'jungle_leaves': (18, 7), 'glass': (20, 0),
    'lapis_ore': (21, 0), 'sandstone': (24, 0), 'smooth_sandstone': (24, 2), 'cobweb': (30, 0), 'short_grass': (31, 1), 'fern': (31, 2), 'dead_bush': (32, 0),
    'dandelion': (37, 0), 'poppy': (38, 0), 'blue_orchid': (38, 1), 'allium': (38, 2), 'oxeye_daisy': (38, 8), 'cornflower': (38, 0), 'brown_mushroom': (39, 0),
    'red_mushroom': (40, 0), 'bricks': (45, 0), 'bookshelf': (47, 0), 'mossy_cobblestone': (48, 0), 'obsidian': (49, 0), 'torch': (50, 5), 'diamond_ore': (56, 0),
    'crafting_table': (58, 0), 'wheat': (59, 7), 'farmland': (60, 7), 'furnace_block': (61, 2), 'redstone_ore': (73, 0), 'snow': (78, 0), 'ice': (79, 0),
    'snow_block': (80, 0), 'cactus': (81, 0), 'clay': (82, 0), 'sugar_cane': (83, 0), 'pumpkin': (86, 0), 'glowstone': (89, 0), 'stone_bricks': (98, 0),
    'melon': (103, 0), 'vine': (106, 0), 'mycelium': (110, 0), 'lily_pad': (111, 0), 'emerald_ore': (129, 0), 'cobblestone_wall': (139, 0), 'flower_pot': (140, 0),
    'terracotta': (172, 0), 'white_terracotta': (159, 0), 'orange_terracotta': (159, 1), 'yellow_terracotta': (159, 4), 'brown_terracotta': (159, 12),
    'red_terracotta': (159, 14), 'hay_block': (170, 0), 'packed_ice': (174, 0), 'acacia_log': (162, 0), 'dark_oak_log': (162, 1), 'acacia_leaves': (161, 4),
    'dark_oak_leaves': (161, 5), 'oak_fence': (85, 0), 'spruce_fence': (188, 0), 'dirt_path': (3, 0), 'lantern': (89, 0), 'lantern_hanging': (89, 0),
    'bed': (26, 0), 'composter': (5, 0), 'barrel': (5, 0), 'tall_grass': (175, 2), 'tall_grass_top': (175, 10), 'large_fern': (175, 3), 'large_fern_top': (175, 10),
    'sunflower': (175, 0), 'sunflower_top': (175, 10), 'lilac': (175, 1), 'lilac_top': (175, 10), 'rose_bush': (175, 4), 'rose_bush_top': (175, 10),
    'peony': (175, 5), 'peony_top': (175, 10),
}
LEGACY_WORDS = [
    ('deepslate_coal_ore', (16, 0)), ('deepslate_iron_ore', (15, 0)), ('deepslate_gold_ore', (14, 0)), ('deepslate_diamond_ore', (56, 0)),
    ('deepslate_redstone_ore', (73, 0)), ('deepslate_lapis_ore', (21, 0)), ('deepslate_emerald_ore', (129, 0)), ('copper_ore', (1, 0)),
    ('cherry_log', (17, 0)), ('mangrove_log', (17, 3)), ('cherry_leaves', (18, 4)), ('mangrove_leaves', (18, 7)), ('azalea_leaves', (18, 4)),
    ('stone_stairs', (109, 0)), ('cobblestone_stairs', (67, 0)), ('oak_stairs', (53, 0)), ('spruce_stairs', (134, 0)), ('sandstone_stairs', (128, 0)),
    ('slab_top', (44, 8)), ('cobblestone_slab', (44, 3)), ('sandstone_slab', (44, 1)), ('stone_slab', (44, 0)), ('oak_slab', (126, 0)), ('spruce_slab', (126, 1)),
    ('door_', (64, 0)), ('shutter', (96, 0)), ('trapdoor', (96, 0)), ('dripstone', (0, 0)), ('cave_vines', (106, 0)), ('glow_lichen', (0, 0)),
    ('sculk_vein', (0, 0)), ('hanging_roots', (0, 0)), ('amethyst_cluster', (0, 0)), ('moss_carpet', (171, 13)), ('azalea', (18, 4)),
    ('deepslate', (1, 0)), ('tuff', (1, 0)), ('calcite', (1, 3)), ('smooth_basalt', (1, 0)), ('moss_block', (48, 0)), ('mud', (3, 1)),
    ('rooted_dirt', (3, 0)), ('powder_snow', (80, 0)), ('amethyst', (155, 0)), ('sculk', (1, 0)), ('magma', (87, 0)), ('ancient_debris', (49, 0)),
    ('white_terracotta_wall', (159, 0)),
]

def state_key(name, props):
    return name + ('[' + ','.join('%s=%s' % kv for kv in sorted(props.items())) + ']' if props else '')

def legacy(key):
    found = LEGACY_EXACT.get(key)
    for word, value in LEGACY_WORDS:
        if found is None and word in key:
            found = value
    found = found or (1, 0)
    return (found[0] << 4) | found[1]

by_id_26_2 = {}
for name, block in json.load(open(os.path.join(HERE, '..', 'reference', 'gen_26_2', 'reports', 'blocks.json'))).items():
    for state in block['states']:
        by_id_26_2[state['id']] = (name, state.get('properties', {}))
ids_26_3 = {}
for name, block in json.load(open(os.path.join(HERE, '..', 'reference', 'gen_26_3', 'reports', 'blocks.json'))).items():
    for state in block['states']:
        ids_26_3[state_key(name, state.get('properties', {}))] = state['id']

rows = []
for line in open(SOURCE):
    key, _, value = line.strip().partition('=')
    if not value.isdigit():
        continue
    name, props = by_id_26_2[int(value)]
    state = state_key(name, props)
    flags = 0 if name in conv.AIR_BLOCKS else conv.light_class(name, props)
    fluid = 1 if name in conv.FLUID_BLOCKS or props.get('waterlogged') == 'true' else 0
    rows.append((key, state, int(value), ids_26_3.get(state, 0), legacy(key), {0: 0, 4: 15, 8: 1}[flags], fluid))

missing = [r[1] for r in rows if r[3] == 0 and r[0] != 'air']
with open(OUT, 'w', newline='\n') as out:
    out.write('# key state id_26_2 id_26_3 legacy_1_8 light_filter fluid\n')
    for row in rows:
        out.write('%s %s %d %d %d %d %d\n' % row)
print(len(rows), 'blocks ->', OUT, 'missing in 26.3:', missing)
print('enum:', ', '.join('block_' + r[0] for r in rows))
