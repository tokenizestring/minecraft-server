
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	generator_c generator;

	void generator_c::set_seed(std::uint64_t seed)
	{
		world_seed = seed;
	}
	/*
	//=====================================================================================
	*/
	void generator_c::generate(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		base_x = chunk_x * 16;
		base_z = chunk_z * 16;
		blocks.fill(structures::block_air);

		auto highest{ world_min_y };

		for (auto column{ 0u }; column < 256u; ++column)
		{
			auto world_x{ base_x + static_cast<std::int32_t>(column & 15u) };
			auto world_z{ base_z + static_cast<std::int32_t>(column >> 4u) };
			heights[column] = surface_height(world_x, world_z);
			biomes[column] = biome_for(world_x, world_z, heights[column]);
			highest = std::max(highest, heights[column]);
		}

		sample_caves(highest);

		for (auto column{ 0u }; column < 256u; ++column)
		{
			fill_column(static_cast<std::int32_t>(column & 15u), static_cast<std::int32_t>(column >> 4u));
		}

		for (auto column{ 0u }; column < 256u; ++column)
		{
			auto y{ std::min(heights[column], world_min_y + world_height - 1) };

			while (y > world_min_y && blocks[index(static_cast<std::int32_t>(column & 15u), y, static_cast<std::int32_t>(column >> 4u))] == structures::block_air)
			{
				y -= 1;
			}

			grounds[column] = y;
		}

		caverns.dress(this, chunk_x, chunk_z);
		features.collect(this, chunk_x, chunk_z);
		features.place(this, chunk_x, chunk_z);
		flora.place_trees(this, chunk_x, chunk_z);
		flora.decorate(this, chunk_x, chunk_z);

		place_veins(chunk_x, chunk_z, structures::block_granite, structures::block_tuff, -60, 80, 14, 34, 21ui64);
		place_veins(chunk_x, chunk_z, structures::block_diorite, structures::block_tuff, -60, 80, 14, 34, 22ui64);
		place_veins(chunk_x, chunk_z, structures::block_andesite, structures::block_tuff, -60, 80, 14, 34, 23ui64);
		place_veins(chunk_x, chunk_z, structures::block_tuff, structures::block_tuff, -64, -20, 12, 40, 24ui64);
		place_veins(chunk_x, chunk_z, structures::block_gravel, structures::block_gravel, -50, 60, 9, 26, 25ui64);
		place_veins(chunk_x, chunk_z, structures::block_dripstone_block, structures::block_dripstone_block, -58, 20, 7, 28, 26ui64);
		place_veins(chunk_x, chunk_z, structures::block_smooth_basalt, structures::block_smooth_basalt, -64, -44, 4, 24, 27ui64);
		place_veins(chunk_x, chunk_z, structures::block_coal_ore, structures::block_deepslate_coal_ore, 8, 128, 22, 14, 1ui64);
		place_veins(chunk_x, chunk_z, structures::block_copper_ore, structures::block_deepslate_copper_ore, -12, 96, 14, 12, 2ui64);
		place_veins(chunk_x, chunk_z, structures::block_iron_ore, structures::block_deepslate_iron_ore, -48, 72, 18, 10, 3ui64);
		place_veins(chunk_x, chunk_z, structures::block_gold_ore, structures::block_deepslate_gold_ore, -56, 24, 8, 8, 4ui64);
		place_veins(chunk_x, chunk_z, structures::block_redstone_ore, structures::block_deepslate_redstone_ore, -60, 8, 10, 9, 5ui64);
		place_veins(chunk_x, chunk_z, structures::block_lapis_ore, structures::block_deepslate_lapis_ore, -56, 20, 6, 7, 6ui64);
		place_veins(chunk_x, chunk_z, structures::block_diamond_ore, structures::block_deepslate_diamond_ore, -62, -16, 5, 6, 7ui64);
		place_veins(chunk_x, chunk_z, structures::block_emerald_ore, structures::block_deepslate_emerald_ore, -60, 40, 3, 3, 8ui64);
		place_veins(chunk_x, chunk_z, structures::block_amethyst_block, structures::block_amethyst_block, -58, -10, 3, 10, 9ui64);
		place_veins(chunk_x, chunk_z, structures::block_glowstone, structures::block_glowstone, -60, -30, 2, 5, 10ui64);
		place_veins(chunk_x, chunk_z, structures::block_sculk, structures::block_sculk, -64, -40, 6, 16, 11ui64);
		place_veins(chunk_x, chunk_z, structures::block_ancient_debris, structures::block_ancient_debris, -64, -50, 1, 2, 12ui64);

		for (auto column{ 0u }; column < 256u; ++column)
		{
			auto y{ world_min_y + world_height - 1 };

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
	void generator_c::apply(const std::vector<structures::edit_s>& list)
	{
		for (const auto& edit : list)
		{
			blocks[edit.index] = static_cast<std::uint8_t>(edit.kind < placed_kind_base ? edit.kind : structures::block_stone);
		}

		for (const auto& edit : list)
		{
			auto column{ edit.index & 255u };
			auto y{ world_min_y + world_height - 1 };

			while (y > world_min_y && blocks[index(static_cast<std::int32_t>(column & 15u), y, static_cast<std::int32_t>(column >> 4u))] == structures::block_air)
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
	structures::spawn_s generator_c::get_spawn() const
	{
		auto spot_x{ 0 };
		auto spot_z{ 0 };
		auto spot_y{ surface_height(0, 0) };
		auto found{ false };

		for (auto distance{ 0 }; distance <= gen_spawn_search && !found; distance += gen_spawn_step)
		{
			for (auto x{ -distance }; x <= distance && !found; x += gen_spawn_step)
			{
				for (auto z{ -distance }; z <= distance && !found; z += std::abs(x) == distance ? gen_spawn_step : distance * 2)
				{
					if (auto height{ surface_height(x, z) }; height > gen_sea_level + 1 && height < gen_snow_line - 14 && carve_density(static_cast<std::float_t>(x), static_cast<std::float_t>(height), static_cast<std::float_t>(z)) < 0.0f)
					{
						spot_x = x;
						spot_y = height;
						spot_z = z;
						found = true;
					}
				}
			}
		}

		return { static_cast<std::double_t>(spot_x) + 0.5, static_cast<std::double_t>(spot_y + 1), static_cast<std::double_t>(spot_z) + 0.5, 0.0f };
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t generator_c::get_seed() const
	{
		return world_seed;
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::smooth(std::float_t t)
	{
		return t * t * (3.0f - 2.0f * t);
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::gradient(std::int64_t x, std::int64_t z, std::uint64_t salt) const
	{
		return static_cast<std::float_t>(functions::mix64(static_cast<std::uint64_t>(x) * 0x9E3779B97F4A7C15ui64 ^ static_cast<std::uint64_t>(z) * 0xC2B2AE3D27D4EB4Fui64 ^ (world_seed + salt)) >> 40u) / 8388608.0f - 1.0f;
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::gradient3(std::int64_t x, std::int64_t y, std::int64_t z, std::uint64_t salt) const
	{
		return static_cast<std::float_t>(functions::mix64(static_cast<std::uint64_t>(x) * 0x9E3779B97F4A7C15ui64 ^ static_cast<std::uint64_t>(y) * 0x85EBCA77C2B2AE63ui64 ^ static_cast<std::uint64_t>(z) * 0xC2B2AE3D27D4EB4Fui64 ^ (world_seed + salt)) >> 40u) / 8388608.0f - 1.0f;
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::noise2(std::float_t x, std::float_t z, std::uint64_t salt) const
	{
		auto base_x{ static_cast<std::int64_t>(std::floor(x)) };
		auto base_z{ static_cast<std::int64_t>(std::floor(z)) };
		auto fraction_x{ smooth(x - static_cast<std::float_t>(base_x)) };
		auto fraction_z{ smooth(z - static_cast<std::float_t>(base_z)) };
		auto first{ gradient(base_x, base_z, salt) };
		auto second{ gradient(base_x, base_z + 1i64, salt) };
		auto top{ first + (gradient(base_x + 1i64, base_z, salt) - first) * fraction_x };
		auto bottom{ second + (gradient(base_x + 1i64, base_z + 1i64, salt) - second) * fraction_x };

		return top + (bottom - top) * fraction_z;
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::noise3(std::float_t x, std::float_t y, std::float_t z, std::uint64_t salt) const
	{
		auto base_x{ static_cast<std::int64_t>(std::floor(x)) };
		auto base_y{ static_cast<std::int64_t>(std::floor(y)) };
		auto base_z{ static_cast<std::int64_t>(std::floor(z)) };
		auto fraction_x{ smooth(x - static_cast<std::float_t>(base_x)) };
		auto fraction_y{ smooth(y - static_cast<std::float_t>(base_y)) };
		auto fraction_z{ smooth(z - static_cast<std::float_t>(base_z)) };
		auto layer{ [&](std::int64_t rise) { auto first{ gradient3(base_x, base_y + rise, base_z, salt) }; auto second{ gradient3(base_x, base_y + rise, base_z + 1i64, salt) }; auto top{ first + (gradient3(base_x + 1i64, base_y + rise, base_z, salt) - first) * fraction_x }; auto bottom{ second + (gradient3(base_x + 1i64, base_y + rise, base_z + 1i64, salt) - second) * fraction_x }; return top + (bottom - top) * fraction_z; } };
		auto lower{ layer(0i64) };

		return lower + (layer(1i64) - lower) * fraction_y;
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::fractal2(std::float_t x, std::float_t z, std::uint32_t octaves, std::float_t frequency, std::uint64_t salt) const
	{
		auto total{ 0.0f };
		auto amplitude{ 1.0f };
		auto sum{ 0.0f };

		for (auto octave{ 0u }; octave < octaves; ++octave)
		{
			total += noise2(x * frequency, z * frequency, salt + static_cast<std::uint64_t>(octave) * 7919ui64) * amplitude;
			sum += amplitude;
			amplitude *= 0.5f;
			frequency *= 2.0f;
		}

		return total / sum;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t generator_c::surface_height(std::int32_t x, std::int32_t z) const
	{
		auto plain_x{ static_cast<std::float_t>(x) };
		auto plain_z{ static_cast<std::float_t>(z) };
		auto warp_x{ plain_x + fractal2(plain_x, plain_z, 2u, 1.0f / 190.0f, 31ui64) * 80.0f };
		auto warp_z{ plain_z + fractal2(plain_x, plain_z, 2u, 1.0f / 190.0f, 37ui64) * 80.0f };
		auto raw_mask{ std::clamp((fractal2(warp_x, warp_z, 3u, 1.0f / 760.0f, 23ui64) - 0.02f) / 0.30f, 0.0f, 1.0f) };
		auto mask{ raw_mask * raw_mask };
		auto ridged{ 1.0f - std::fabs(fractal2(warp_x, warp_z, 5u, 1.0f / 260.0f, 19ui64)) };
		auto peaks{ ridged * ridged * ridged * mask * 185.0f };

		if (mask > 0.25f)
		{
			peaks += std::floor(fractal2(warp_x, warp_z, 2u, 1.0f / 90.0f, 29ui64) * 4.0f) * 3.0f;
		}

		return std::clamp(static_cast<std::int32_t>(static_cast<std::float_t>(gen_sea_level) + 12.0f + fractal2(warp_x, warp_z, 4u, 1.0f / 1100.0f, 11ui64) * 30.0f + fractal2(warp_x, warp_z, 4u, 1.0f / 240.0f, 13ui64) * 17.0f + fractal2(warp_x, warp_z, 3u, 1.0f / 64.0f, 17ui64) * 5.0f + peaks), world_min_y + 6, world_min_y + world_height - 40);
	}
	/*
	//=====================================================================================
	*/
	structures::biome_e generator_c::biome_at(std::int32_t x, std::int32_t z) const
	{
		return biome_for(x, z, surface_height(x, z));
	}
	/*
	//=====================================================================================
	*/
	structures::biome_e generator_c::biome_for(std::int32_t x, std::int32_t z, std::int32_t height) const
	{
		auto temperature{ fractal2(static_cast<std::float_t>(x), static_cast<std::float_t>(z), 3u, 1.0f / 720.0f, 101ui64) };
		auto humidity{ fractal2(static_cast<std::float_t>(x), static_cast<std::float_t>(z), 3u, 1.0f / 560.0f, 202ui64) };
		auto oddity{ fractal2(static_cast<std::float_t>(x), static_cast<std::float_t>(z), 2u, 1.0f / 1400.0f, 303ui64) };
		auto biome{ oddity > 0.25f ? structures::biome_meadow : structures::biome_plains };

		if (height < gen_sea_level - 2)
		{
			biome = oddity > 0.48f ? structures::biome_mushroom_fields : structures::biome_ocean;
		}

		else if (height <= gen_sea_level + 1)
		{
			biome = structures::biome_beach;
		}

		else if (height > gen_snow_line + 18)
		{
			biome = structures::biome_jagged_peaks;
		}

		else if (height > gen_snow_line - 14)
		{
			biome = temperature < 0.0f ? structures::biome_jagged_peaks : structures::biome_stony_peaks;
		}

		else if (temperature > 0.17f && humidity < -0.14f)
		{
			biome = structures::biome_desert;
		}

		else if (temperature > 0.17f && humidity < 0.10f)
		{
			biome = oddity > 0.10f ? structures::biome_badlands : structures::biome_savanna;
		}

		else if (temperature > 0.17f)
		{
			biome = structures::biome_jungle;
		}

		else if (temperature < -0.26f)
		{
			biome = humidity > 0.05f ? structures::biome_snowy_taiga : structures::biome_snowy_plains;
		}

		else if (humidity > 0.18f)
		{
			biome = height < gen_sea_level + 6 ? structures::biome_swamp : structures::biome_dark_forest;
		}

		else if (humidity > 0.06f)
		{
			biome = oddity > 0.15f ? structures::biome_birch_forest : structures::biome_forest;
		}

		else if (humidity > -0.08f)
		{
			biome = oddity < -0.20f ? structures::biome_cherry_grove : structures::biome_taiga;
		}

		return biome;
	}
	/*
	//=====================================================================================
	*/
	structures::biome_e generator_c::get_biome(std::uint32_t column) const
	{
		return biomes[column];
	}
	/*
	//=====================================================================================
	*/
	std::int32_t generator_c::get_ground(std::uint32_t column) const
	{
		return grounds[column];
	}
	/*
	//=====================================================================================
	*/
	bool generator_c::site_clear(std::int32_t world_x, std::int32_t world_z) const
	{
		return features.site_clear(world_x, world_z);
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t generator_c::get_block(std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		auto block{ static_cast<std::uint8_t>(structures::block_air) };

		if (x >= 0 && x < 16 && z >= 0 && z < 16 && y >= world_min_y && y < world_min_y + world_height)
		{
			block = blocks[index(x, y, z)];
		}

		return block;
	}
	/*
	//=====================================================================================
	*/
	void generator_c::set_block(std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t block)
	{
		if (x >= 0 && x < 16 && z >= 0 && z < 16 && y >= world_min_y && y < world_min_y + world_height)
		{
			blocks[index(x, y, z)] = block;
		}
	}
	/*
	//=====================================================================================
	*/
	bool generator_c::inside(std::int32_t world_x, std::int32_t world_z) const
	{
		return (world_x >= base_x && world_x < base_x + 16 && world_z >= base_z && world_z < base_z + 16);
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t generator_c::peek(std::int32_t world_x, std::int32_t y, std::int32_t world_z) const
	{
		auto block{ static_cast<std::uint8_t>(structures::block_stone) };

		if (y > world_min_y && y < world_min_y + world_height && inside(world_x, world_z))
		{
			block = blocks[index(world_x - base_x, y, world_z - base_z)];
		}

		return block;
	}
	/*
	//=====================================================================================
	*/
	void generator_c::solid(std::int32_t world_x, std::int32_t y, std::int32_t world_z, std::uint8_t block)
	{
		if (y > world_min_y && y < world_min_y + world_height && inside(world_x, world_z))
		{
			blocks[index(world_x - base_x, y, world_z - base_z)] = block;
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::soft(std::int32_t world_x, std::int32_t y, std::int32_t world_z, std::uint8_t block)
	{
		if (y > world_min_y && y < world_min_y + world_height && inside(world_x, world_z))
		{
			if (auto& cell{ blocks[index(world_x - base_x, y, world_z - base_z)] }; cell == structures::block_air)
			{
				cell = block;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::tunnel_band(std::float_t x, std::float_t y, std::float_t z, std::float_t horizontal, std::float_t vertical, std::float_t width, std::float_t fill, std::uint64_t salt) const
	{
		return std::max(0.0f, width - std::fabs(noise3(x / horizontal, y / vertical, z / horizontal, salt))) * std::max(0.0f, width - std::fabs(noise3(x / horizontal, y / vertical, z / horizontal, salt + 7ui64))) / (width * width * fill) - 1.0f;
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::carve_density(std::float_t x, std::float_t y, std::float_t z) const
	{
		auto density{ -1.0f };

		if (y >= static_cast<std::float_t>(world_min_y + 2))
		{
			auto deep{ std::clamp((16.0f - y) / 78.0f, 0.0f, 1.0f) };
			auto tighten{ deep * 0.026f };

			density = tunnel_band(x, y, z, 148.0f, 30.0f, 0.082f - tighten * 0.5f, 0.235f, 401ui64);
			density = std::max(density, tunnel_band(x, y, z, 88.0f, 18.0f, 0.074f - tighten * 0.5f, 0.225f, 411ui64));
			density = std::max(density, tunnel_band(x, y, z, 240.0f, 48.0f, 0.068f - tighten * 0.3f, 0.210f, 421ui64));
			density = std::max(density, tunnel_band(x, y, z, 60.0f, 13.0f, 0.048f + deep * 0.014f, 0.200f, 461ui64));
			density += noise3(x / 8.5f, y / 6.5f, z / 8.5f, 481ui64) * 0.62f + noise3(x / 3.5f, y / 3.0f, z / 3.5f, 482ui64) * 0.24f;

			if (y < -18.0f)
			{
				auto region{ noise3(x / 196.0f, y / 74.0f, z / 196.0f, 403ui64) };
				auto gate{ 0.48f - std::min(1.0f, (-18.0f - y) / 44.0f) * 0.08f };

				if (region > gate)
				{
					density = std::max(density, (noise3(x / 33.0f, y / 19.0f, z / 33.0f, 404ui64) - (0.70f - std::min(1.0f, (region - gate) / 0.20f) * 0.20f + deep * 0.10f)) * 4.0f);
				}
			}

			if (y > -58.0f && y < 46.0f)
			{
				auto seam{ std::fabs(noise2(x / 210.0f, z / 210.0f, 501ui64)) };
				auto width{ 0.009f + noise2(x / 70.0f, z / 70.0f, 502ui64) * 0.006f * 0.6f };
				auto centre{ (y + 6.0f) / 52.0f };

				if (seam < width && 1.0f - centre * centre > 0.0f)
				{
					density = std::max(density, ((width - seam) / width * (1.0f - centre * centre) * 3.4f) - 0.55f);
				}
			}
		}

		return density;
	}
	/*
	//=====================================================================================
	*/
	bool generator_c::lava_here(std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		return (y < gen_lava_level && noise2(static_cast<std::float_t>(x) / 124.0f, static_cast<std::float_t>(z) / 124.0f, 451ui64) + noise2(static_cast<std::float_t>(x) / 31.0f, static_cast<std::float_t>(z) / 31.0f, 452ui64) * 0.06f > 0.42f - static_cast<std::float_t>(gen_lava_level - y) / 6.0f * 0.10f);
	}
	/*
	//=====================================================================================
	*/
	void generator_c::sample_caves(std::int32_t highest)
	{
		auto levels{ std::min(gen_cave_levels, (highest - world_min_y) / gen_cave_step + 2) };

		for (auto grid_z{ 0 }; grid_z < gen_cave_span; ++grid_z)
		{
			for (auto grid_x{ 0 }; grid_x < gen_cave_span; ++grid_x)
			{
				for (auto level{ 0 }; level < levels; ++level)
				{
					caves[static_cast<std::size_t>((level * gen_cave_span + grid_z) * gen_cave_span + grid_x)] = carve_density(static_cast<std::float_t>(base_x + grid_x * gen_cave_step), static_cast<std::float_t>(world_min_y + level * gen_cave_step), static_cast<std::float_t>(base_z + grid_z * gen_cave_step));
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::float_t generator_c::cave_at(std::int32_t local_x, std::int32_t y, std::int32_t local_z) const
	{
		auto level{ (y - world_min_y) / gen_cave_step };
		auto value{ -1.0f };

		if (level >= 0 && level < gen_cave_levels - 1)
		{
			auto fraction_x{ static_cast<std::float_t>(local_x % gen_cave_step) / static_cast<std::float_t>(gen_cave_step) };
			auto fraction_z{ static_cast<std::float_t>(local_z % gen_cave_step) / static_cast<std::float_t>(gen_cave_step) };
			auto fraction_y{ static_cast<std::float_t>((y - world_min_y) % gen_cave_step) / static_cast<std::float_t>(gen_cave_step) };
			auto layer{ [&](std::int32_t rise) { auto cell{ static_cast<std::size_t>(((level + rise) * gen_cave_span + local_z / gen_cave_step) * gen_cave_span + local_x / gen_cave_step) }; auto top{ caves[cell] + (caves[cell + 1u] - caves[cell]) * fraction_x }; auto bottom{ caves[cell + gen_cave_span] + (caves[cell + gen_cave_span + 1u] - caves[cell + gen_cave_span]) * fraction_x }; return top + (bottom - top) * fraction_z; } };
			auto lower{ layer(0) };
			value = lower + (layer(1) - lower) * fraction_y;
		}

		return value;
	}
	/*
	//=====================================================================================
	*/
	void generator_c::fill_column(std::int32_t local_x, std::int32_t local_z)
	{
		auto column{ static_cast<std::uint32_t>(local_z * 16 + local_x) };
		auto height{ heights[column] };

		blocks[index(local_x, world_min_y, local_z)] = structures::block_bedrock;

		for (auto y{ world_min_y + 1 }; y <= std::max(height, gen_sea_level); ++y)
		{
			auto block{ y <= gen_deepslate_top ? structures::block_deepslate : structures::block_stone };

			if (y > height)
			{
				block = structures::block_water;
			}

			else if (cave_at(local_x, y, local_z) > 0.0f)
			{
				block = lava_here(base_x + local_x, y, base_z + local_z) ? structures::block_lava : structures::block_air;
			}

			blocks[index(local_x, y, local_z)] = block;
		}

		surface_column(local_x, local_z, height, biomes[column]);
	}
	/*
	//=====================================================================================
	*/
	void generator_c::surface_column(std::int32_t local_x, std::int32_t local_z, std::int32_t height, structures::biome_e biome)
	{
		auto top{ structures::block_grass_block };
		auto filler{ structures::block_dirt };
		auto depth{ 4 };

		switch (biome)
		{
		case structures::biome_desert:
			top = structures::block_sand;
			filler = structures::block_sandstone;
			depth = 6;
			break;

		case structures::biome_beach:
			top = structures::block_sand;
			filler = structures::block_sandstone;
			break;

		case structures::biome_badlands:
			top = structures::block_red_terracotta;
			filler = structures::block_orange_terracotta;
			depth = 12;
			break;

		case structures::biome_savanna:
			filler = structures::block_coarse_dirt;
			break;

		case structures::biome_snowy_plains:
		case structures::biome_snowy_taiga:
			top = structures::block_snow_block;
			break;

		case structures::biome_taiga:
		case structures::biome_dark_forest:
			top = structures::block_podzol;
			break;

		case structures::biome_swamp:
			filler = structures::block_mud;
			depth = 5;
			break;

		case structures::biome_mushroom_fields:
			top = structures::block_mycelium;
			break;

		case structures::biome_ocean:
			top = structures::block_gravel;
			filler = structures::block_clay;
			depth = 3;
			break;

		case structures::biome_stony_peaks:
			top = structures::block_stone;
			filler = structures::block_tuff;
			depth = 3;
			break;

		case structures::biome_jagged_peaks:
			top = structures::block_snow_block;
			filler = structures::block_stone;
			depth = 3;
			break;
		}

		if (height < gen_sea_level)
		{
			top = structures::block_gravel;
		}

		else if (height >= gen_snow_line && biome != structures::biome_jagged_peaks && biome != structures::biome_stony_peaks)
		{
			top = structures::block_snow_block;
		}

		if (get_block(local_x, height, local_z) != structures::block_air)
		{
			set_block(local_x, height, local_z, top);
		}

		for (auto y{ height - 1 }; y >= height - depth && y > world_min_y; --y)
		{
			if (get_block(local_x, y, local_z) == structures::block_stone)
			{
				set_block(local_x, y, local_z, filler);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void generator_c::place_veins(std::int32_t chunk_x, std::int32_t chunk_z, std::uint8_t stone, std::uint8_t deepslate, std::int32_t min_y, std::int32_t max_y, std::int32_t attempts, std::int32_t size, std::uint64_t salt)
	{
		auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 341873128712ui64 + static_cast<std::uint64_t>(chunk_z) * 132897987541ui64 + world_seed + salt) };

		for (auto attempt{ 0 }; attempt < attempts; ++attempt)
		{
			state = functions::mix64(state + gen_golden);

			if (max_y > min_y)
			{
				auto origin_x{ static_cast<std::int32_t>(state % 16ui64) };
				auto origin_z{ static_cast<std::int32_t>((state >> 8u) % 16ui64) };
				auto origin_y{ min_y + static_cast<std::int32_t>((state >> 16u) % static_cast<std::uint64_t>(max_y - min_y)) };

				for (auto block{ 0 }; block < size; ++block)
				{
					state = functions::mix64(state + gen_weyl);

					auto x{ origin_x + static_cast<std::int32_t>((state >> 4u) % 3ui64) - 1 };
					auto y{ origin_y + static_cast<std::int32_t>((state >> 12u) % 3ui64) - 1 };
					auto z{ origin_z + static_cast<std::int32_t>((state >> 20u) % 3ui64) - 1 };

					if (x >= 0 && x <= 15 && z >= 0 && z <= 15 && y > world_min_y && y < world_min_y + world_height)
					{
						if (auto& cell{ blocks[index(x, y, z)] }; cell == structures::block_stone)
						{
							cell = stone;
						}

						else if (cell == structures::block_deepslate)
						{
							cell = deepslate;
						}
					}
				}
			}
		}
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
