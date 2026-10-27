
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	void flora_c::place_trees(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		owner = parent;
		trees.clear();

		for (auto offset_z{ -1 }; offset_z <= 1; ++offset_z)
		{
			for (auto offset_x{ -1 }; offset_x <= 1; ++offset_x)
			{
				collect_trees(chunk_x + offset_x, chunk_z + offset_z);
			}
		}

		for (const auto& plan : trees)
		{
			if (auto reach{ tree_reach(plan.kind) }; plan.x + reach >= chunk_x * 16 && plan.x - reach < chunk_x * 16 + 16 && plan.z + reach >= chunk_z * 16 && plan.z - reach < chunk_z * 16 + 16 && owner->site_clear(plan.x, plan.z))
			{
				build_tree(plan);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::decorate(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		owner = parent;

		auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 512927377ui64 + static_cast<std::uint64_t>(chunk_z) * 233478511ui64 + owner->get_seed() + 909ui64) };
		auto patch_flower{ patch_flower_for(functions::mix64(state + 7717ui64)) };

		for (auto attempt{ 0 }; attempt < gen_plant_attempts; ++attempt)
		{
			state = functions::mix64(state + gen_weyl);

			auto local_x{ static_cast<std::int32_t>(state % 16ui64) };
			auto local_z{ static_cast<std::int32_t>((state >> 8u) % 16ui64) };
			auto column{ static_cast<std::uint32_t>(local_z * 16 + local_x) };

			if (auto surface{ owner->get_ground(column) }; surface >= gen_sea_level - 4 && surface <= gen_snow_line)
			{
				while (surface + 1 < world_min_y + world_height && owner->get_block(local_x, surface + 1, local_z) == structures::block_water)
				{
					surface += 1;
				}

				auto roll{ static_cast<std::uint32_t>((state >> 24u) % 100ui64) };

				if (owner->get_block(local_x, surface, local_z) == structures::block_water)
				{
					if (roll < 14u && owner->get_biome(column) != structures::biome_ocean)
					{
						owner->set_block(local_x, surface + 1, local_z, structures::block_lily_pad);
					}
				}

				else if (owner->get_block(local_x, surface + 1, local_z) == structures::block_air)
				{
					plant(chunk_x * 16 + local_x, surface, chunk_z * 16 + local_z, roll, state, patch_flower);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::plant(std::int32_t world_x, std::int32_t surface, std::int32_t world_z, std::uint32_t roll, std::uint64_t state, std::uint8_t patch_flower)
	{
		auto local_x{ world_x & 15 };
		auto local_z{ world_z & 15 };
		auto ground{ owner->get_block(local_x, surface, local_z) };
		auto biome{ owner->get_biome(static_cast<std::uint32_t>(local_z * 16 + local_x)) };
		auto meadow{ owner->noise2(static_cast<std::float_t>(world_x) / 54.0f, static_cast<std::float_t>(world_z) / 54.0f, 771ui64) };
		auto lush{ meadow >= -0.06f && static_cast<std::float_t>(roll) / 100.0f <= std::clamp((meadow + 0.06f) / 0.26f, 0.0f, 1.0f) * (0.72f + owner->noise2(static_cast<std::float_t>(world_x) / 11.0f, static_cast<std::float_t>(world_z) / 11.0f, 772ui64) * 0.28f) };
		auto tall_pick{ (state >> 44u) % 4ui64 };
		auto growth{ static_cast<std::uint8_t>(structures::block_air) };

		if ((ground == structures::block_sand || ground == structures::block_grass_block || ground == structures::block_dirt) && roll < 55u && water_beside(local_x, surface, local_z))
		{
			for (auto step{ 0 }; step < 2 + static_cast<std::int32_t>((state >> 36u) % 3ui64); ++step)
			{
				owner->set_block(local_x, surface + 1 + step, local_z, structures::block_sugar_cane);
			}
		}

		else
		{
			switch (biome)
			{
			case structures::biome_desert:
				if (ground == structures::block_sand && roll < 20u)
				{
					growth = structures::block_dead_bush;
				}

				else if (ground == structures::block_sand && roll < 26u)
				{
					for (auto offset{ 0 }; offset < 2 + static_cast<std::int32_t>(roll % 2u); ++offset)
					{
						owner->set_block(local_x, surface + 1 + offset, local_z, structures::block_cactus);
					}
				}

				break;

			case structures::biome_badlands:
				growth = roll < 30u ? structures::block_dead_bush : structures::block_air;
				break;

			case structures::biome_taiga:
			case structures::biome_snowy_taiga:
				if (lush && roll < 12u)
				{
					tall_plant(local_x, surface + 1, local_z, structures::block_large_fern, structures::block_large_fern_top);
				}

				else if (lush)
				{
					growth = roll < 55u ? structures::block_fern : structures::block_short_grass;
				}

				else
				{
					growth = roll < 40u ? structures::block_fern : structures::block_short_grass;
				}

				break;

			case structures::biome_jungle:
				if (lush && roll < 30u)
				{
					tall_plant(local_x, surface + 1, local_z, structures::block_tall_grass, structures::block_tall_grass_top);
				}

				else if (lush)
				{
					growth = roll < 55u ? structures::block_fern : structures::block_short_grass;
				}

				else
				{
					growth = roll < 35u ? structures::block_fern : structures::block_short_grass;
				}

				break;

			case structures::biome_swamp:
				growth = roll < 18u ? structures::block_brown_mushroom : (roll < 24u ? structures::block_blue_orchid : structures::block_short_grass);
				break;

			case structures::biome_mushroom_fields:
				growth = roll < 50u ? structures::block_red_mushroom : structures::block_brown_mushroom;
				break;

			case structures::biome_stony_peaks:
			case structures::biome_jagged_peaks:
				break;

			default:
				if (ground == structures::block_grass_block && !lush)
				{
					growth = structures::block_short_grass;
				}

				else if (ground == structures::block_grass_block && roll < 6u)
				{
					tall_plant(local_x, surface + 1, local_z, tall_pick == 1ui64 ? structures::block_lilac : (tall_pick == 2ui64 ? structures::block_peony : (tall_pick == 3ui64 ? structures::block_sunflower : structures::block_rose_bush)), tall_pick == 1ui64 ? structures::block_lilac_top : (tall_pick == 2ui64 ? structures::block_peony_top : (tall_pick == 3ui64 ? structures::block_sunflower_top : structures::block_rose_bush_top)));
				}

				else if (ground == structures::block_grass_block && roll < 14u)
				{
					tall_plant(local_x, surface + 1, local_z, structures::block_tall_grass, structures::block_tall_grass_top);
				}

				else if (ground == structures::block_grass_block && roll < 30u)
				{
					growth = patch_flower;
				}

				else if (ground == structures::block_grass_block && roll < 34u)
				{
					growth = ((state >> 50u) % 2ui64) ? structures::block_azalea : structures::block_flowering_azalea;
				}

				else if (ground == structures::block_grass_block && roll < 38u && (biome == structures::biome_plains || biome == structures::biome_meadow || biome == structures::biome_forest))
				{
					growth = structures::block_pumpkin;
				}

				else if (ground == structures::block_grass_block)
				{
					growth = structures::block_short_grass;
				}

				break;
			}
		}

		if (growth != structures::block_air)
		{
			owner->set_block(local_x, surface + 1, local_z, growth);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::tall_plant(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint8_t lower, std::uint8_t upper)
	{
		if (y + 1 < world_min_y + world_height && owner->get_block(local_x, y + 1, local_z) == structures::block_air)
		{
			owner->set_block(local_x, y, local_z, lower);
			owner->set_block(local_x, y + 1, local_z, upper);
		}
	}
	/*
	//=====================================================================================
	*/
	bool flora_c::water_beside(std::int32_t local_x, std::int32_t y, std::int32_t local_z) const
	{
		auto beside{ false };

		for (auto side{ 0u }; side < 4u && !beside; ++side)
		{
			beside = (owner->get_block(local_x + gen_side_x[side], y, local_z + gen_side_z[side]) == structures::block_water);
		}

		return beside;
	}
	/*
	//=====================================================================================
	*/
	void flora_c::collect_trees(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 738295147ui64 + static_cast<std::uint64_t>(chunk_z) * 918273641ui64 + owner->get_seed() + 77ui64) };

		for (auto attempt{ 0 }; attempt < gen_tree_attempts; ++attempt)
		{
			state = functions::mix64(state + gen_golden);

			auto world_x{ chunk_x * 16 + static_cast<std::int32_t>(state % 16ui64) };
			auto world_z{ chunk_z * 16 + static_cast<std::int32_t>((state >> 8u) % 16ui64) };
			auto ground{ owner->surface_height(world_x, world_z) };
			auto biome{ owner->biome_for(world_x, world_z, ground) };

			if (auto kind{ kind_for(biome, state >> 32u) }; static_cast<std::int32_t>((state >> 20u) % 14ui64) < density_for(biome) && ground >= gen_sea_level && ground <= gen_snow_line && kind != structures::tree_none)
			{
				trees.push_back({ world_x, world_z, ground, kind, functions::mix64(state ^ 0xA5A5A5A5A5A5A5A5ui64) });
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::build_tree(const structures::tree_plan_s& plan)
	{
		carve_tree(plan);
		anchor_tree(plan.x, plan.z, plan.ground + 1, wood_for(plan.kind).log);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_tree(const structures::tree_plan_s& plan)
	{
		switch (plan.kind)
		{
		case structures::tree_bush:
			carve_bush(plan);
			break;

		case structures::tree_oak_ancient:
			carve_ancient(plan);
			break;

		case structures::tree_spruce_mega:
			carve_mega_spruce(plan);
			break;

		case structures::tree_willow:
			carve_willow(plan);
			break;

		case structures::tree_birch_tall:
			carve_tall_birch(plan);
			break;

		case structures::tree_spruce:
		case structures::tree_spruce_tall:
			carve_spruce(plan);
			break;

		case structures::tree_acacia:
			carve_acacia(plan);
			break;

		case structures::tree_oak_small:
		case structures::tree_oak_grand:
		case structures::tree_birch:
		case structures::tree_jungle_giant:
		case structures::tree_dark_oak:
		case structures::tree_cherry:
			carve_broadleaf(plan);
			break;
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_bush(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto stem{ 1 + static_cast<std::int32_t>(plan.seed % 2ui64) };

		for (auto step{ 0 }; step < stem; ++step)
		{
			owner->solid(plan.x, plan.ground + 1 + step, plan.z, wood.log);
		}

		leaf_blob(plan.x, plan.ground + 1 + stem, plan.z, 2, wood.leaves, plan.seed);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_ancient(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto base_y{ plan.ground + 1 };
		auto trunk{ 15 + static_cast<std::int32_t>(plan.seed % 9ui64) };
		auto arms{ 5 + static_cast<std::int32_t>((plan.seed >> 12u) % 3ui64) };

		root_flare(plan.x, base_y, plan.z, 4, wood.log, plan.seed + 313ui64);
		root_flare(plan.x + 1, base_y, plan.z + 1, 3, wood.log, plan.seed + 707ui64);
		taper_trunk(plan.x, base_y, plan.z, trunk, 1, wood.log, plan.seed);

		for (auto arm{ 0 }; arm < arms; ++arm)
		{
			auto arm_seed{ functions::mix64(plan.seed + static_cast<std::uint64_t>(arm) * 7919ui64) };
			auto angle{ 6.2831853 * static_cast<std::double_t>(arm) / static_cast<std::double_t>(arms) + static_cast<std::double_t>(arm_seed % 90ui64) / 100.0 };

			limb(plan.x, base_y + trunk - 6 + static_cast<std::int32_t>(arm_seed % 6ui64), plan.z, std::cos(angle), std::sin(angle), 5 + static_cast<std::int32_t>((arm_seed >> 8u) % 4ui64), 3 + static_cast<std::int32_t>((arm_seed >> 16u) % 3ui64), wood.log, wood.leaves, arm_seed, 4, 1);
		}

		canopy_cluster(plan.x, base_y + trunk + 1, plan.z, 4, 6, 4, wood.leaves, plan.seed + 55ui64);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_mega_spruce(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto base_y{ plan.ground + 1 };
		auto height{ 22 + static_cast<std::int32_t>(plan.seed % 11ui64) };

		root_flare(plan.x, base_y, plan.z, 3, wood.log, plan.seed + 881ui64);

		for (auto step{ 0 }; step < height; ++step)
		{
			owner->solid(plan.x, base_y + step, plan.z, wood.log);
			owner->solid(plan.x + 1, base_y + step, plan.z, wood.log);
			owner->solid(plan.x, base_y + step, plan.z + 1, wood.log);
			owner->solid(plan.x + 1, base_y + step, plan.z + 1, wood.log);
		}

		for (auto layer{ 0 }; layer < height - 4; ++layer)
		{
			auto radius{ std::min(5, 1 + ((height - 4 - layer) % 7) / 2) };

			for (auto offset_z{ -radius }; offset_z <= radius + 1; ++offset_z)
			{
				for (auto offset_x{ -radius }; offset_x <= radius + 1; ++offset_x)
				{
					if (std::abs(offset_x) + std::abs(offset_z) <= radius + 2)
					{
						owner->soft(plan.x + offset_x, base_y + 4 + layer, plan.z + offset_z, wood.leaves);
					}
				}
			}
		}

		for (auto cap{ 0 }; cap < 3; ++cap)
		{
			owner->soft(plan.x, base_y + height + cap, plan.z, wood.leaves);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_willow(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto base_y{ plan.ground + 1 };
		auto trunk{ 6 + static_cast<std::int32_t>(plan.seed % 4ui64) };
		auto arms{ 5 + static_cast<std::int32_t>((plan.seed >> 10u) % 3ui64) };

		root_flare(plan.x, base_y, plan.z, 3, wood.log, plan.seed + 211ui64);
		taper_trunk(plan.x, base_y, plan.z, trunk, 1, wood.log, plan.seed);

		for (auto arm{ 0 }; arm < arms; ++arm)
		{
			auto arm_seed{ functions::mix64(plan.seed + static_cast<std::uint64_t>(arm) * 4409ui64) };
			auto angle{ 6.2831853 * static_cast<std::double_t>(arm) / static_cast<std::double_t>(arms) };
			auto length{ 4 + static_cast<std::int32_t>(arm_seed % 4ui64) };

			limb(plan.x, base_y + trunk - 1, plan.z, std::cos(angle), std::sin(angle), length, 1, wood.log, wood.leaves, arm_seed, 3, 0);
			hang_vines(plan.x + static_cast<std::int32_t>(std::round(std::cos(angle) * length)), base_y + trunk - 2, plan.z + static_cast<std::int32_t>(std::round(std::sin(angle) * length)), 4 + static_cast<std::int32_t>((arm_seed >> 8u) % 5ui64), arm_seed + 33ui64);
		}

		canopy_cluster(plan.x, base_y + trunk, plan.z, 3, 4, 3, wood.leaves, plan.seed + 17ui64);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_tall_birch(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto base_y{ plan.ground + 1 };
		auto trunk{ 11 + static_cast<std::int32_t>(plan.seed % 5ui64) };

		for (auto step{ 0 }; step < trunk; ++step)
		{
			owner->solid(plan.x, base_y + step, plan.z, wood.log);
		}

		for (auto arm{ 0 }; arm < 3; ++arm)
		{
			auto arm_seed{ functions::mix64(plan.seed + static_cast<std::uint64_t>(arm) * 3313ui64) };
			auto angle{ 6.2831853 * static_cast<std::double_t>(arm) / 3.0 };

			limb(plan.x, base_y + trunk - 3 - static_cast<std::int32_t>(arm_seed % 3ui64), plan.z, std::cos(angle), std::sin(angle), 2 + static_cast<std::int32_t>(arm_seed % 2ui64), 2, wood.log, wood.leaves, arm_seed, 2, 0);
		}

		canopy_cluster(plan.x, base_y + trunk, plan.z, 2, 3, 3, wood.leaves, plan.seed + 5ui64);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_spruce(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto base_y{ plan.ground + 1 };
		auto height{ plan.kind == structures::tree_spruce_tall ? 17 + static_cast<std::int32_t>(plan.seed % 9ui64) : 10 + static_cast<std::int32_t>(plan.seed % 5ui64) };
		auto bare{ 1 + static_cast<std::int32_t>((plan.seed >> 4u) % 3ui64) };
		auto widest{ 2 + static_cast<std::int32_t>((plan.seed >> 8u) % 2ui64) + (plan.kind == structures::tree_spruce_tall ? 1 : 0) };

		for (auto step{ 0 }; step < height; ++step)
		{
			owner->solid(plan.x, base_y + step, plan.z, wood.log);
		}

		for (auto from_top{ 0 }; from_top <= height - bare; ++from_top)
		{
			auto radius{ std::min((from_top + 1) / 3 - from_top % 2, widest) };

			for (auto offset_z{ -radius }; offset_z <= radius; ++offset_z)
			{
				for (auto offset_x{ -radius }; offset_x <= radius; ++offset_x)
				{
					if (auto squared{ offset_x * offset_x + offset_z * offset_z }; squared <= radius * radius + radius && (squared <= (radius - 1) * (radius - 1) + radius || !functions::one_in(functions::mix64(plan.seed + static_cast<std::uint64_t>(from_top * 71 + offset_x * 13 + offset_z * 29)), 4ui64)))
					{
						owner->soft(plan.x + offset_x, base_y + height - from_top, plan.z + offset_z, wood.leaves);
					}
				}
			}
		}

		owner->soft(plan.x, base_y + height + 1, plan.z, wood.leaves);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_acacia(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto base_y{ plan.ground + 1 };
		auto trunk{ 5 + static_cast<std::int32_t>(plan.seed % 3ui64) };
		auto arms{ 2 + static_cast<std::int32_t>((plan.seed >> 8u) % 2ui64) };

		for (auto step{ 0 }; step < trunk; ++step)
		{
			owner->solid(plan.x, base_y + step, plan.z, wood.log);
		}

		for (auto arm{ 0 }; arm < arms; ++arm)
		{
			auto arm_seed{ functions::mix64(plan.seed + static_cast<std::uint64_t>(arm) * 5501ui64) };
			auto angle{ static_cast<std::double_t>(arm_seed % 628ui64) / 100.0 };

			branch(plan.x, base_y + trunk - 1, plan.z, std::cos(angle), std::sin(angle), 3 + static_cast<std::int32_t>(arm_seed % 3ui64), 2, wood.log, wood.leaves, arm_seed, 3);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::carve_broadleaf(const structures::tree_plan_s& plan)
	{
		auto wood{ wood_for(plan.kind) };
		auto seed{ plan.seed };
		auto base_y{ plan.ground + 1 };
		auto jungle{ plan.kind == structures::tree_jungle_giant };
		auto giant{ jungle || plan.kind == structures::tree_oak_grand || plan.kind == structures::tree_dark_oak || plan.kind == structures::tree_cherry };
		auto thick{ jungle || plan.kind == structures::tree_dark_oak || (plan.kind == structures::tree_oak_grand && functions::one_in(seed, 2ui64)) };
		auto trunk{ 5 + static_cast<std::int32_t>(seed % 3ui64) };

		switch (plan.kind)
		{
		case structures::tree_jungle_giant:
			trunk = 16 + static_cast<std::int32_t>(seed % 8ui64);
			break;

		case structures::tree_oak_grand:
			trunk = 10 + static_cast<std::int32_t>(seed % 6ui64);
			break;

		case structures::tree_dark_oak:
			trunk = 8 + static_cast<std::int32_t>(seed % 4ui64);
			break;

		case structures::tree_cherry:
		case structures::tree_birch:
			trunk = 7 + static_cast<std::int32_t>(seed % 4ui64);
			break;
		}

		if (giant)
		{
			root_flare(plan.x, base_y, plan.z, jungle ? 3 : 2, wood.log, seed + 991ui64);
		}

		for (auto step{ 0 }; step < trunk; ++step)
		{
			owner->solid(plan.x, base_y + step, plan.z, wood.log);

			if (thick)
			{
				owner->solid(plan.x + 1, base_y + step, plan.z, wood.log);
				owner->solid(plan.x, base_y + step, plan.z + 1, wood.log);
				owner->solid(plan.x + 1, base_y + step, plan.z + 1, wood.log);
			}
		}

		if (giant)
		{
			auto arms{ 3 + static_cast<std::int32_t>((seed >> 12u) % 4ui64) };
			auto blob{ jungle ? 4 : 3 };

			for (auto arm{ 0 }; arm < arms; ++arm)
			{
				auto arm_seed{ functions::mix64(seed + static_cast<std::uint64_t>(arm) * 7919ui64) };
				auto angle{ static_cast<std::double_t>(arm_seed % 628ui64) / 100.0 };

				branch(plan.x, base_y + trunk - 2 - static_cast<std::int32_t>(arm_seed % 4ui64), plan.z, std::cos(angle), std::sin(angle), (jungle ? 4 : 3) + static_cast<std::int32_t>((arm_seed >> 8u) % 3ui64), 2 + static_cast<std::int32_t>((arm_seed >> 16u) % 3ui64), wood.log, wood.leaves, arm_seed, blob);
			}

			leaf_blob(plan.x, base_y + trunk + 1, plan.z, blob + 1, wood.leaves, seed + 5ui64);

			if (thick)
			{
				leaf_blob(plan.x + 1, base_y + trunk + 1, plan.z + 1, blob, wood.leaves, seed + 9ui64);
			}

			for (auto vine{ 0 }; vine < 8 && jungle; ++vine)
			{
				auto vine_seed{ functions::mix64(seed + 3000ui64 + static_cast<std::uint64_t>(vine)) };
				hang_vines(plan.x + static_cast<std::int32_t>(vine_seed % 11ui64) - 5, base_y + trunk - 1, plan.z + static_cast<std::int32_t>((vine_seed >> 8u) % 11ui64) - 5, 5 + static_cast<std::int32_t>((vine_seed >> 16u) % 8ui64), vine_seed);
			}
		}

		else
		{
			leaf_blob(plan.x, base_y + trunk, plan.z, plan.kind == structures::tree_birch ? 2 : 3, wood.leaves, seed);
			owner->soft(plan.x, base_y + trunk + 1, plan.z, wood.leaves);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::anchor_tree(std::int32_t x, std::int32_t z, std::int32_t base_y, std::uint8_t log)
	{
		for (auto offset_z{ 0 }; offset_z <= 1; ++offset_z)
		{
			for (auto offset_x{ 0 }; offset_x <= 1; ++offset_x)
			{
				if (owner->peek(x + offset_x, base_y, z + offset_z) == log)
				{
					for (auto y{ base_y - 1 }; y >= base_y - 14 && open_below(x + offset_x, y, z + offset_z); --y)
					{
						owner->solid(x + offset_x, y, z + offset_z, log);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	bool flora_c::open_below(std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		auto block{ owner->peek(x, y, z) };
		return (block == structures::block_air || block == structures::block_water);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::leaf_blob(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t radius, std::uint8_t leaves, std::uint64_t seed)
	{
		for (auto offset_y{ -radius }; offset_y <= radius; ++offset_y)
		{
			for (auto offset_z{ -radius }; offset_z <= radius; ++offset_z)
			{
				for (auto offset_x{ -radius }; offset_x <= radius; ++offset_x)
				{
					if (auto squared{ offset_x * offset_x + offset_y * offset_y + offset_z * offset_z }; squared <= radius * radius && (squared <= (radius - 1) * (radius - 1) || !functions::one_in(functions::mix64(seed + static_cast<std::uint64_t>(offset_x * 31 + offset_y * 71 + offset_z * 131)), 5ui64)))
					{
						owner->soft(x + offset_x, y + offset_y, z + offset_z, leaves);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::hang_vines(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t length, std::uint64_t seed)
	{
		for (auto step{ 1 }; step <= length && !functions::one_in(functions::mix64(seed + static_cast<std::uint64_t>(step)), 8ui64); ++step)
		{
			owner->soft(x, y - step, z, structures::block_vine);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::branch(std::int32_t x, std::int32_t y, std::int32_t z, std::double_t dir_x, std::double_t dir_z, std::int32_t length, std::int32_t rise, std::uint8_t log, std::uint8_t leaves, std::uint64_t seed, std::int32_t blob)
	{
		auto cursor_x{ static_cast<std::double_t>(x) };
		auto cursor_y{ static_cast<std::double_t>(y) };
		auto cursor_z{ static_cast<std::double_t>(z) };
		auto climb{ static_cast<std::double_t>(rise) / static_cast<std::double_t>(std::max(1, length)) };

		for (auto step{ 0 }; step < length; ++step)
		{
			cursor_x += dir_x;
			cursor_z += dir_z;
			cursor_y += climb;
			owner->solid(static_cast<std::int32_t>(std::round(cursor_x)), static_cast<std::int32_t>(std::round(cursor_y)), static_cast<std::int32_t>(std::round(cursor_z)), log);
		}

		auto tip_x{ static_cast<std::int32_t>(std::round(cursor_x)) };
		auto tip_y{ static_cast<std::int32_t>(std::round(cursor_y)) };
		auto tip_z{ static_cast<std::int32_t>(std::round(cursor_z)) };

		leaf_blob(tip_x, tip_y, tip_z, blob, leaves, seed);

		if (functions::one_in(seed, 3ui64))
		{
			hang_vines(tip_x, tip_y - blob, tip_z, 3 + static_cast<std::int32_t>(seed % 5ui64), seed + 17ui64);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::root_flare(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t spread, std::uint8_t log, std::uint64_t seed)
	{
		for (auto spoke{ 0 }; spoke < 8; ++spoke)
		{
			auto angle{ static_cast<std::double_t>(spoke) * 0.7853981634 };
			auto step_x{ std::cos(angle) };
			auto step_z{ std::sin(angle) };
			auto reach{ 1 + static_cast<std::int32_t>(functions::mix64(seed + static_cast<std::uint64_t>(spoke)) % static_cast<std::uint64_t>(spread + 1)) };

			for (auto step{ 1 }; step <= reach; ++step)
			{
				auto root_x{ x + static_cast<std::int32_t>(std::round(step_x * step)) };
				auto root_z{ z + static_cast<std::int32_t>(std::round(step_z * step)) };

				for (auto at{ y }; at >= y - 6 && open_below(root_x, at, root_z); --at)
				{
					owner->solid(root_x, at, root_z, log);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::canopy_cluster(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t spread, std::int32_t blobs, std::int32_t radius, std::uint8_t leaves, std::uint64_t seed)
	{
		for (auto blob{ 0 }; blob < blobs; ++blob)
		{
			auto blob_seed{ functions::mix64(seed + static_cast<std::uint64_t>(blob) * 6607ui64) };
			auto angle{ static_cast<std::double_t>(blob_seed % 628ui64) / 100.0 };
			auto reach{ static_cast<std::double_t>(blob_seed % static_cast<std::uint64_t>(spread + 1)) };

			leaf_blob(x + static_cast<std::int32_t>(std::round(std::cos(angle) * reach)), y + static_cast<std::int32_t>((blob_seed >> 12u) % 4ui64) - 1, z + static_cast<std::int32_t>(std::round(std::sin(angle) * reach)), std::max(2, radius - static_cast<std::int32_t>((blob_seed >> 20u) % 2ui64)), leaves, blob_seed);
		}
	}
	/*
	//=====================================================================================
	*/
	void flora_c::limb(std::int32_t x, std::int32_t y, std::int32_t z, std::double_t dir_x, std::double_t dir_z, std::int32_t length, std::int32_t rise, std::uint8_t log, std::uint8_t leaves, std::uint64_t seed, std::int32_t blob, std::int32_t depth)
	{
		auto cursor_x{ static_cast<std::double_t>(x) };
		auto cursor_y{ static_cast<std::double_t>(y) };
		auto cursor_z{ static_cast<std::double_t>(z) };
		auto climb{ static_cast<std::double_t>(rise) / static_cast<std::double_t>(std::max(1, length)) };
		auto drift{ static_cast<std::double_t>(static_cast<std::int64_t>(seed % 200ui64) - 100i64) / 2400.0 };
		auto heading_x{ dir_x };
		auto heading_z{ dir_z };

		for (auto step{ 0 }; step < length; ++step)
		{
			heading_x += drift * heading_z;
			heading_z -= drift * heading_x;
			cursor_x += heading_x;
			cursor_z += heading_z;
			cursor_y += climb;
			owner->solid(static_cast<std::int32_t>(std::round(cursor_x)), static_cast<std::int32_t>(std::round(cursor_y)), static_cast<std::int32_t>(std::round(cursor_z)), log);
		}

		auto tip_x{ static_cast<std::int32_t>(std::round(cursor_x)) };
		auto tip_y{ static_cast<std::int32_t>(std::round(cursor_y)) };
		auto tip_z{ static_cast<std::int32_t>(std::round(cursor_z)) };

		if (depth > 0 && length > 3)
		{
			auto fork{ functions::mix64(seed + 4241ui64) };
			auto angle{ static_cast<std::double_t>(fork % 628ui64) / 100.0 };

			limb(tip_x, tip_y, tip_z, std::cos(angle) * 0.9, std::sin(angle) * 0.9, length / 2 + 1, 1, log, leaves, fork, std::max(2, blob - 1), depth - 1);
		}

		canopy_cluster(tip_x, tip_y, tip_z, 2, 3, blob, leaves, seed + 71ui64);
	}
	/*
	//=====================================================================================
	*/
	void flora_c::taper_trunk(std::int32_t x, std::int32_t y, std::int32_t z, std::int32_t height, std::int32_t base_width, std::uint8_t log, std::uint64_t seed)
	{
		for (auto step{ 0 }; step < height; ++step)
		{
			auto width{ step > height / 2 ? 0 : (step > height / 4 ? base_width - 1 : base_width) };

			for (auto offset_z{ 0 }; offset_z <= width; ++offset_z)
			{
				for (auto offset_x{ 0 }; offset_x <= width; ++offset_x)
				{
					if (width < 1 || offset_x != width || offset_z != width || !functions::one_in(functions::mix64(seed + static_cast<std::uint64_t>(step)), 3ui64))
					{
						owner->solid(x + offset_x, y + step, z + offset_z, log);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	structures::wood_s flora_c::wood_for(structures::tree_e kind)
	{
		structures::wood_s wood{ structures::block_oak_log, structures::block_oak_leaves };

		switch (kind)
		{
		case structures::tree_birch:
		case structures::tree_birch_tall:
			wood = { structures::block_birch_log, structures::block_birch_leaves };
			break;

		case structures::tree_spruce:
		case structures::tree_spruce_tall:
		case structures::tree_spruce_mega:
			wood = { structures::block_spruce_log, structures::block_spruce_leaves };
			break;

		case structures::tree_jungle_giant:
			wood = { structures::block_jungle_log, structures::block_jungle_leaves };
			break;

		case structures::tree_acacia:
			wood = { structures::block_acacia_log, structures::block_acacia_leaves };
			break;

		case structures::tree_dark_oak:
			wood = { structures::block_dark_oak_log, structures::block_dark_oak_leaves };
			break;

		case structures::tree_cherry:
			wood = { structures::block_cherry_log, structures::block_cherry_leaves };
			break;
		}

		return wood;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t flora_c::tree_reach(structures::tree_e kind)
	{
		auto reach{ 5 };

		switch (kind)
		{
		case structures::tree_oak_ancient:
			reach = 16;
			break;

		case structures::tree_jungle_giant:
			reach = 14;
			break;

		case structures::tree_willow:
			reach = 13;
			break;

		case structures::tree_spruce_mega:
		case structures::tree_oak_grand:
		case structures::tree_dark_oak:
		case structures::tree_cherry:
			reach = 10;
			break;

		case structures::tree_acacia:
			reach = 9;
			break;

		case structures::tree_birch_tall:
			reach = 7;
			break;

		case structures::tree_spruce_tall:
			reach = 6;
			break;

		case structures::tree_bush:
			reach = 3;
			break;
		}

		return reach;
	}
	/*
	//=====================================================================================
	*/
	structures::tree_e flora_c::kind_for(structures::biome_e biome, std::uint64_t roll)
	{
		auto pick{ roll % 100ui64 };
		auto kind{ structures::tree_none };

		switch (biome)
		{
		case structures::biome_forest:
			kind = pick < 6ui64 ? structures::tree_oak_ancient : (pick < 30ui64 ? structures::tree_oak_grand : (pick < 40ui64 ? structures::tree_bush : structures::tree_oak_small));
			break;

		case structures::biome_plains:
			kind = pick < 8ui64 ? structures::tree_oak_ancient : (pick < 45ui64 ? structures::tree_oak_grand : structures::tree_oak_small);
			break;

		case structures::biome_meadow:
			kind = pick < 10ui64 ? structures::tree_oak_ancient : (pick < 55ui64 ? structures::tree_oak_grand : structures::tree_oak_small);
			break;

		case structures::biome_birch_forest:
			kind = pick < 40ui64 ? structures::tree_birch_tall : structures::tree_birch;
			break;

		case structures::biome_dark_forest:
			kind = pick < 12ui64 ? structures::tree_oak_ancient : (pick < 70ui64 ? structures::tree_dark_oak : structures::tree_bush);
			break;

		case structures::biome_cherry_grove:
			kind = structures::tree_cherry;
			break;

		case structures::biome_taiga:
			kind = pick < 18ui64 ? structures::tree_spruce_mega : (pick < 60ui64 ? structures::tree_spruce_tall : structures::tree_spruce);
			break;

		case structures::biome_snowy_taiga:
			kind = pick < 14ui64 ? structures::tree_spruce_mega : (pick < 65ui64 ? structures::tree_spruce_tall : structures::tree_spruce);
			break;

		case structures::biome_snowy_plains:
			kind = structures::tree_spruce;
			break;

		case structures::biome_jungle:
			kind = pick < 55ui64 ? structures::tree_jungle_giant : (pick < 70ui64 ? structures::tree_bush : structures::tree_oak_small);
			break;

		case structures::biome_savanna:
			kind = structures::tree_acacia;
			break;

		case structures::biome_swamp:
			kind = pick < 70ui64 ? structures::tree_willow : structures::tree_oak_grand;
			break;
		}

		return kind;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t flora_c::density_for(structures::biome_e biome)
	{
		auto density{ 0 };

		switch (biome)
		{
		case structures::biome_forest:
		case structures::biome_dark_forest:
			density = 10;
			break;

		case structures::biome_birch_forest:
		case structures::biome_taiga:
		case structures::biome_snowy_taiga:
		case structures::biome_jungle:
			density = 9;
			break;

		case structures::biome_cherry_grove:
			density = 7;
			break;

		case structures::biome_swamp:
			density = 6;
			break;

		case structures::biome_savanna:
			density = 3;
			break;

		case structures::biome_meadow:
		case structures::biome_plains:
			density = 2;
			break;

		case structures::biome_snowy_plains:
			density = 1;
			break;
		}

		return density;
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t flora_c::patch_flower_for(std::uint64_t patch)
	{
		auto flower{ structures::block_poppy };

		switch (patch % 6ui64)
		{
		case 1ui64:
			flower = structures::block_dandelion;
			break;

		case 2ui64:
			flower = structures::block_cornflower;
			break;

		case 3ui64:
			flower = structures::block_oxeye_daisy;
			break;

		case 4ui64:
			flower = structures::block_allium;
			break;

		case 5ui64:
			flower = structures::block_blue_orchid;
			break;
		}

		return flower;
	}
}

//=====================================================================================
