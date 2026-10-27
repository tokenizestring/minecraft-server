
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	generator_c generator;

	void generator_c::set_seed(std::uint64_t seed)
	{
		world_seed = seed;
		seed_low = static_cast<std::uint32_t>(seed ^ (seed >> 32u));
	}
	/*
	//=====================================================================================
	*/
	void generator_c::generate(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		fill(chunk_x, chunk_z);
		carve(chunk_x, chunk_z);
		place_ores(chunk_x, chunk_z);
		decorate(chunk_x, chunk_z);
		place_trees(chunk_x, chunk_z);

		for (auto column{ 0u }; column < 256u; ++column)
		{
			auto y{ std::min(std::max(surfaces[column], smp_sea_level) + 16, world_min_y + world_height - 1) };

			while (y >= world_min_y && blocks[index(static_cast<std::int32_t>(column & 15u), y, static_cast<std::int32_t>(column >> 4u))] == structures::block_air)
			{
				y -= 1;
			}

			tops[column] = y;
		}
	}
	/*
	//=====================================================================================
	*/
	const std::uint8_t* generator_c::get_blocks() const
	{
		return blocks.data();
	}
	/*
	//=====================================================================================
	*/
	std::int32_t generator_c::get_top(std::uint32_t column) const
	{
		return tops[column];
	}
	/*
	//=====================================================================================
	*/
	std::int32_t generator_c::get_surface(std::int32_t x, std::int32_t z) const
	{
		auto continent{ fractal(x, z, 4u, 1.0 / 720.0, seed_low + 11u) };
		auto hills{ fractal(x, z, 4u, 1.0 / 160.0, seed_low + 23u) };
		auto mask{ std::clamp((fractal(x, z, 3u, 1.0 / 900.0, seed_low + 37u) - 0.08) * 3.0, 0.0, 1.0) };
		auto ridge{ 1.0 - std::abs(fractal(x, z, 4u, 1.0 / 260.0, seed_low + 41u)) };
		auto land{ std::clamp((continent + 0.12) * 4.0, 0.0, 1.0) };
		auto height{ 64.0 + continent * 34.0 + hills * (3.0 + 9.0 * land) + mask * land * ridge * ridge * 95.0 };

		return std::clamp(static_cast<std::int32_t>(height), world_min_y + 8, 300);
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t generator_c::get_biome(std::int32_t x, std::int32_t z, std::int32_t surface) const
	{
		auto temperature{ fractal(x, z, 2u, 1.0 / 1200.0, seed_low + 53u) };
		auto humidity{ fractal(x, z, 2u, 1.0 / 900.0, seed_low + 67u) };
		auto biome{ biome_plains };

		if (surface < smp_sea_level - 1)
		{
			biome = biome_ocean;
		}

		else if (surface > 128)
		{
			biome = biome_mountain;
		}

		else if (surface <= smp_sea_level + 1 && temperature > -0.18)
		{
			biome = biome_beach;
		}

		else if (temperature < -0.18)
		{
			biome = biome_snowy;
		}

		else if (temperature > 0.18 && humidity < 0.05)
		{
			biome = biome_desert;
		}

		else if (humidity > 0.08)
		{
			biome = biome_forest;
		}

		return biome;
	}
	/*
	//=====================================================================================
	*/
	structures::spawn_s generator_c::get_spawn() const
	{
		auto point{ find_spawn() };
		return { static_cast<std::double_t>(point[0]) + 0.5, static_cast<std::double_t>(point[1]), static_cast<std::double_t>(point[2]) + 0.5, 0.0f };
	}
	/*
	//=====================================================================================
	*/
	std::array<std::int32_t, 3u> generator_c::find_spawn() const
	{
		std::array<std::int32_t, 3u> spawn{ 0, get_surface(0, 0) + 1, 0 };
		auto found{ false };

		for (auto distance{ 0 }; distance <= smp_spawn_search && found == false; distance += 8)
		{
			for (auto x{ -distance }; x <= distance && found == false; x += 8)
			{
				for (auto z : { -distance, distance })
				{
					if (auto surface{ get_surface(x, z) }; found == false && surface > smp_sea_level && get_biome(x, z, surface) != biome_ocean && get_biome(x, z, surface) != biome_mountain)
					{
						spawn = { x, surface + 1, z };
						found = true;
					}
				}
			}
		}

		return spawn;
	}
	/*
	//=====================================================================================
	*/
	void generator_c::fill(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		blocks.fill(structures::block_air);

		for (auto z{ 0 }; z < 16; ++z)
		{
			for (auto x{ 0 }; x < 16; ++x)
			{
				auto world_x{ chunk_x * 16 + x };
				auto world_z{ chunk_z * 16 + z };
				auto surface{ get_surface(world_x, world_z) };
				auto biome{ get_biome(world_x, world_z, surface) };
				auto column{ static_cast<std::uint32_t>(z * 16 + x) };
				surfaces[column] = surface;
				biomes[column] = biome;

				for (auto y{ world_min_y }; y <= std::max(surface, smp_sea_level - 1); ++y)
				{
					auto noise{ hash(world_x, y, world_z, seed_low) };
					auto depth{ surface - y };
					auto block{ structures::block_water };

					if (y < world_min_y + 5 && noise % 5u >= static_cast<std::uint32_t>(y - world_min_y))
					{
						block = structures::block_bedrock;
					}

					else if (depth > 3)
					{
						block = (y < 0 || (y < 8 && static_cast<std::int32_t>(noise % 8u) > y)) ? structures::block_deepslate : structures::block_stone;
					}

					else if (depth > 0 && (biome == biome_desert || biome == biome_beach))
					{
						block = depth < 3 ? structures::block_sand : structures::block_sandstone;
					}

					else if (depth > 0 && biome == biome_ocean)
					{
						block = structures::block_sand;
					}

					else if (depth > 0 && biome == biome_mountain)
					{
						block = structures::block_stone;
					}

					else if (depth > 0)
					{
						block = structures::block_dirt;
					}

					else if (depth == 0 && biome == biome_ocean)
					{
						block = surface < smp_sea_level - 12 ? structures::block_gravel : ((noise & 7u) ? structures::block_sand : structures::block_clay);
					}

					else if (depth == 0 && (biome == biome_desert || biome == biome_beach))
					{
						block = structures::block_sand;
					}

					else if (depth == 0 && biome == biome_mountain)
					{
						block = surface > 150 ? structures::block_snow_block : structures::block_stone;
					}

					else if (depth == 0)
					{
						block = structures::block_grass;
					}

					else if (biome == biome_snowy && y == smp_sea_level - 1)
					{
						block = structures::block_ice;
					}

					blocks[index(x, y, z)] = block;
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::carve(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		for (auto y{ 0 }; y < 49; ++y)
		{
			for (auto z{ 0 }; z < 5; ++z)
			{
				for (auto x{ 0 }; x < 5; ++x)
				{
					caves[static_cast<std::size_t>((y * 5 + z) * 5 + x)] = get_cave(chunk_x * 16 + x * 4, world_min_y + y * 8, chunk_z * 16 + z * 4);
				}
			}
		}

		for (auto z{ 0 }; z < 16; ++z)
		{
			for (auto x{ 0 }; x < 16; ++x)
			{
				auto fraction_x{ static_cast<std::double_t>(x & 3) * 0.25 };
				auto fraction_z{ static_cast<std::double_t>(z & 3) * 0.25 };

				for (auto y{ smp_cave_floor }; y < surfaces[static_cast<std::size_t>(z * 16 + x)] - 5; ++y)
				{
					auto cell{ (y - world_min_y) >> 3 };
					auto fraction_y{ static_cast<std::double_t>((y - world_min_y) & 7) * 0.125 };
					auto sample{ [this, cell, x, z](std::int32_t dy, std::int32_t dz, std::int32_t dx) { return caves[static_cast<std::size_t>(((cell + dy) * 5 + (z >> 2) + dz) * 5 + (x >> 2) + dx)]; } };
					auto low{ std::lerp(std::lerp(sample(0, 0, 0), sample(0, 0, 1), fraction_x), std::lerp(sample(0, 1, 0), sample(0, 1, 1), fraction_x), fraction_z) };
					auto high{ std::lerp(std::lerp(sample(1, 0, 0), sample(1, 0, 1), fraction_x), std::lerp(sample(1, 1, 0), sample(1, 1, 1), fraction_x), fraction_z) };

					if (std::lerp(low, high, fraction_y) < 0.0 && blocks[index(x, y, z)] != structures::block_water)
					{
						blocks[index(x, y, z)] = y < smp_lava_level ? structures::block_lava : structures::block_air;
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::place_ores(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto random{ world_seed ^ (static_cast<std::uint64_t>(hash(chunk_x, 3, chunk_z, seed_low)) << 32u) };

		for (auto ore{ 0u }; ore < std::size(ore_blocks); ++ore)
		{
			for (auto vein{ 0u }; vein < ore_veins[ore]; ++vein)
			{
				auto x{ static_cast<std::int32_t>(next(random) % 16u) };
				auto z{ static_cast<std::int32_t>(next(random) % 16u) };
				auto y{ ore_bottoms[ore] + static_cast<std::int32_t>(next(random) % static_cast<std::uint64_t>(ore_tops[ore] - ore_bottoms[ore])) };

				for (auto step{ 0u }; step < ore_sizes[ore]; ++step)
				{
					auto inside{ x >= 0 && x < 16 && z >= 0 && z < 16 && y >= world_min_y && y < world_min_y + world_height };

					if (inside && blocks[index(x, y, z)] == structures::block_stone)
					{
						blocks[index(x, y, z)] = static_cast<std::uint8_t>(ore_blocks[ore]);
					}

					else if (inside && blocks[index(x, y, z)] == structures::block_deepslate)
					{
						blocks[index(x, y, z)] = static_cast<std::uint8_t>(ore_blocks[ore] + deepslate_ore_offset);
					}

					auto walk{ next(random) };
					x += static_cast<std::int32_t>(walk % 3u) - 1;
					y += static_cast<std::int32_t>((walk >> 8u) % 3u) - 1;
					z += static_cast<std::int32_t>((walk >> 16u) % 3u) - 1;
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::decorate(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		for (auto z{ 0 }; z < 16; ++z)
		{
			for (auto x{ 0 }; x < 16; ++x)
			{
				auto column{ static_cast<std::size_t>(z * 16 + x) };
				auto surface{ surfaces[column] };
				auto top{ blocks[index(x, surface, z)] };
				auto roll{ hash(chunk_x * 16 + x, 1, chunk_z * 16 + z, seed_low + 101u) % 1000u };

				if (top == structures::block_grass && biomes[column] == biome_snowy)
				{
					put(x, surface + 1, z, structures::block_snow, false);
				}

				else if (top == structures::block_grass && roll < 120u)
				{
					put(x, surface + 1, z, structures::block_short_grass, false);
				}

				else if (top == structures::block_grass && roll < 140u)
				{
					put(x, surface + 1, z, structures::block_dandelion, false);
				}

				else if (top == structures::block_grass && roll < 155u)
				{
					put(x, surface + 1, z, structures::block_poppy, false);
				}

				else if (top == structures::block_sand && biomes[column] == biome_desert && roll < 5u)
				{
					for (auto height{ 1u }; height <= 1u + roll % 3u; ++height)
					{
						put(x, surface + static_cast<std::int32_t>(height), z, structures::block_cactus, false);
					}
				}

				else if (top == structures::block_sand && biomes[column] == biome_desert && roll < 12u)
				{
					put(x, surface + 1, z, structures::block_dead_bush, false);
				}

				else if (top == structures::block_snow_block)
				{
					put(x, surface + 1, z, structures::block_snow, false);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::place_trees(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		for (auto near_x{ chunk_x - 1 }; near_x <= chunk_x + 1; ++near_x)
		{
			for (auto near_z{ chunk_z - 1 }; near_z <= chunk_z + 1; ++near_z)
			{
				auto random{ world_seed ^ (static_cast<std::uint64_t>(hash(near_x, 7, near_z, seed_low)) << 32u) ^ static_cast<std::uint64_t>(hash(near_z, 9, near_x, seed_low)) };
				auto middle{ get_surface(near_x * 16 + 8, near_z * 16 + 8) };
				auto count{ biome_tree_counts[get_biome(near_x * 16 + 8, near_z * 16 + 8, middle)] + static_cast<std::uint32_t>(next(random) % 2u) };

				for (auto tree{ 0u }; tree < count; ++tree)
				{
					auto x{ near_x * 16 + static_cast<std::int32_t>(next(random) % 16u) };
					auto z{ near_z * 16 + static_cast<std::int32_t>(next(random) % 16u) };
					auto surface{ get_surface(x, z) };
					auto biome{ get_biome(x, z, surface) };
					auto kind{ (biome == biome_snowy || biome == biome_mountain) ? structures::block_spruce_log : ((biome == biome_forest && next(random) % 4u == 0u) ? structures::block_birch_log : structures::block_oak_log) };

					if (surface > smp_sea_level && surface < 140 && (biome == biome_plains || biome == biome_forest || biome == biome_snowy || biome == biome_mountain))
					{
						place_tree(x - chunk_x * 16, surface + 1, z - chunk_z * 16, kind, random);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::place_tree(std::int32_t base_x, std::int32_t base_y, std::int32_t base_z, std::uint32_t kind, std::uint64_t& random)
	{
		auto leaves{ static_cast<std::uint8_t>(kind + 1u) };
		auto spruce{ kind == structures::block_spruce_log };
		auto trunk{ static_cast<std::int32_t>((spruce ? 6u : 4u) + next(random) % 3u) };

		put(base_x, base_y - 1, base_z, structures::block_dirt, true);

		for (auto layer{ spruce ? 2 : trunk - 3 }; layer <= trunk; ++layer)
		{
			auto radius{ spruce ? ((trunk - layer) % 2 == 0 ? 1 : std::min(2, 1 + (trunk - layer) / 3)) : (layer >= trunk - 1 ? 1 : 2) };

			for (auto x{ -radius }; x <= radius; ++x)
			{
				for (auto z{ -radius }; z <= radius; ++z)
				{
					if (std::abs(x) != radius || std::abs(z) != radius || (radius == 2 && next(random) % 2u == 0u))
					{
						put(base_x + x, base_y + layer, base_z + z, leaves, false);
					}
				}
			}
		}

		put(base_x, base_y + trunk + 1, base_z, leaves, false);

		for (auto y{ 0 }; y < trunk; ++y)
		{
			put(base_x, base_y + y, base_z, static_cast<std::uint8_t>(kind), true);
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::put(std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t block, bool replace)
	{
		if (x >= 0 && x < 16 && z >= 0 && z < 16 && y >= world_min_y && y < world_min_y + world_height)
		{
			if (auto& cell{ blocks[index(x, y, z)] }; (replace && cell != structures::block_bedrock) || cell == structures::block_air || cell == structures::block_short_grass || cell == structures::block_snow)
			{
				cell = block;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::double_t generator_c::get_cave(std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		auto first{ noise_3d(x / 48.0, y / 28.0, z / 48.0, seed_low + 71u) };
		auto second{ noise_3d(x / 48.0, y / 28.0, z / 48.0, seed_low + 83u) };
		auto cavern{ 0.62 - noise_3d(x / 96.0, y / 56.0, z / 96.0, seed_low + 97u) };

		return std::min(first * first + second * second - 0.012, cavern);
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t generator_c::hash(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t seed)
	{
		auto value{ seed ^ (static_cast<std::uint32_t>(x) * 0x27D4EB2Du) ^ (static_cast<std::uint32_t>(y) * 0x165667B1u) ^ (static_cast<std::uint32_t>(z) * 0x9E3779B1u) };
		value ^= value >> 15u;
		value *= 0x85EBCA6Bu;
		value ^= value >> 13u;
		value *= 0xC2B2AE35u;
		value ^= value >> 16u;

		return value;
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t generator_c::next(std::uint64_t& state)
	{
		state += 0x9E3779B97F4A7C15ui64;
		auto value{ state };
		value = (value ^ (value >> 30u)) * 0xBF58476D1CE4E5B9ui64;
		value = (value ^ (value >> 27u)) * 0x94D049BB133111EBui64;

		return value ^ (value >> 31u);
	}
	/*
	//=====================================================================================
	*/
	std::double_t generator_c::noise_2d(std::double_t x, std::double_t z, std::uint32_t seed)
	{
		auto base_x{ std::floor(x) };
		auto base_z{ std::floor(z) };
		auto cell_x{ static_cast<std::int32_t>(base_x) };
		auto cell_z{ static_cast<std::int32_t>(base_z) };
		auto offset_x{ x - base_x };
		auto offset_z{ z - base_z };
		auto corner{ [seed](std::int32_t at_x, std::int32_t at_z, std::double_t delta_x, std::double_t delta_z) { const auto& gradient{ noise_gradients_2d[hash(at_x, 0, at_z, seed) & 7u] }; return gradient[0] * delta_x + gradient[1] * delta_z; } };
		auto fade_x{ offset_x * offset_x * offset_x * (offset_x * (offset_x * 6.0 - 15.0) + 10.0) };
		auto fade_z{ offset_z * offset_z * offset_z * (offset_z * (offset_z * 6.0 - 15.0) + 10.0) };
		auto bottom{ std::lerp(corner(cell_x, cell_z, offset_x, offset_z), corner(cell_x + 1, cell_z, offset_x - 1.0, offset_z), fade_x) };
		auto top{ std::lerp(corner(cell_x, cell_z + 1, offset_x, offset_z - 1.0), corner(cell_x + 1, cell_z + 1, offset_x - 1.0, offset_z - 1.0), fade_x) };

		return std::lerp(bottom, top, fade_z) * 1.414;
	}
	/*
	//=====================================================================================
	*/
	std::double_t generator_c::noise_3d(std::double_t x, std::double_t y, std::double_t z, std::uint32_t seed)
	{
		auto base_x{ std::floor(x) };
		auto base_y{ std::floor(y) };
		auto base_z{ std::floor(z) };
		auto cell_x{ static_cast<std::int32_t>(base_x) };
		auto cell_y{ static_cast<std::int32_t>(base_y) };
		auto cell_z{ static_cast<std::int32_t>(base_z) };
		auto offset_x{ x - base_x };
		auto offset_y{ y - base_y };
		auto offset_z{ z - base_z };
		auto corner{ [seed, cell_x, cell_y, cell_z, offset_x, offset_y, offset_z](std::int32_t dx, std::int32_t dy, std::int32_t dz) { const auto& gradient{ noise_gradients_3d[hash(cell_x + dx, cell_y + dy, cell_z + dz, seed) & 15u] }; return gradient[0] * (offset_x - dx) + gradient[1] * (offset_y - dy) + gradient[2] * (offset_z - dz); } };
		auto fade{ [](std::double_t t) { return t * t * t * (t * (t * 6.0 - 15.0) + 10.0); } };
		auto fade_x{ fade(offset_x) };
		auto fade_y{ fade(offset_y) };
		auto fade_z{ fade(offset_z) };
		auto low{ std::lerp(std::lerp(corner(0, 0, 0), corner(1, 0, 0), fade_x), std::lerp(corner(0, 0, 1), corner(1, 0, 1), fade_x), fade_z) };
		auto high{ std::lerp(std::lerp(corner(0, 1, 0), corner(1, 1, 0), fade_x), std::lerp(corner(0, 1, 1), corner(1, 1, 1), fade_x), fade_z) };

		return std::lerp(low, high, fade_y);
	}
	/*
	//=====================================================================================
	*/
	std::double_t generator_c::fractal(std::double_t x, std::double_t z, std::uint32_t octaves, std::double_t frequency, std::uint32_t seed)
	{
		auto total{ 0.0 };
		auto amplitude{ 1.0 };
		auto weight{ 0.0 };

		for (auto octave{ 0u }; octave < octaves; ++octave)
		{
			total += noise_2d(x * frequency, z * frequency, seed + octave * 0x9E37u) * amplitude;
			weight += amplitude;
			amplitude *= 0.5;
			frequency *= 2.0;
		}

		return total / weight;
	}
	/*
	//=====================================================================================
	*/
	std::size_t generator_c::index(std::int32_t x, std::int32_t y, std::int32_t z)
	{
		return static_cast<std::size_t>(((y - world_min_y) * 16 + z) * 16 + x);
	}
}

//=====================================================================================
