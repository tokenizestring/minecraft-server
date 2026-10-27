
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	void features_c::collect(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		owner = parent;
		lakes.clear();
		fields.clear();
		pyramids.clear();

		for (auto offset_z{ -gen_feature_reach }; offset_z <= gen_feature_reach; ++offset_z)
		{
			for (auto offset_x{ -gen_feature_reach }; offset_x <= gen_feature_reach; ++offset_x)
			{
				collect_lake(chunk_x + offset_x, chunk_z + offset_z);
				collect_field(chunk_x + offset_x, chunk_z + offset_z);
				collect_pyramid(chunk_x + offset_x, chunk_z + offset_z);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void features_c::place(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		owner = parent;

		for (const auto& lake : lakes)
		{
			if (is_near(lake.chunk_x, lake.chunk_z, chunk_x, chunk_z))
			{
				carve_lake(lake);
			}
		}

		for (const auto& field : fields)
		{
			if (is_near(field.chunk_x, field.chunk_z, chunk_x, chunk_z))
			{
				build_field(field);
			}
		}

		for (const auto& pyramid : pyramids)
		{
			if (is_near(pyramid.chunk_x, pyramid.chunk_z, chunk_x, chunk_z))
			{
				build_pyramid(pyramid);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	bool features_c::site_clear(std::int32_t world_x, std::int32_t world_z) const
	{
		auto blocked{ std::ranges::any_of(lakes, [world_x, world_z](const auto& lake) { return is_near(lake.chunk_x, lake.chunk_z, world_x >> 4, world_z >> 4) && ellipse(world_x - lake.x, world_z - lake.z, lake.radius_x + 2, lake.radius_z + 2) <= 1.0; }) };

		blocked = blocked || std::ranges::any_of(fields, [world_x, world_z](const auto& field) { return is_near(field.chunk_x, field.chunk_z, world_x >> 4, world_z >> 4) && std::abs(world_x - field.x) <= field.radius_x + 2 && std::abs(world_z - field.z) <= field.radius_z + 2; });
		blocked = blocked || std::ranges::any_of(pyramids, [world_x, world_z](const auto& pyramid) { return is_near(pyramid.chunk_x, pyramid.chunk_z, world_x >> 4, world_z >> 4) && std::abs(world_x - pyramid.x) <= pyramid.reach + 2 && std::abs(world_z - pyramid.z) <= pyramid.reach + 2; });

		return !blocked;
	}
	/*
	//=====================================================================================
	*/
	void features_c::collect_lake(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		if (auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 419271133ui64 + static_cast<std::uint64_t>(chunk_z) * 663291799ui64 + owner->get_seed() + 4211ui64) }; state % 100ui64 < 7ui64)
		{
			auto world_x{ chunk_x * 16 + static_cast<std::int32_t>((state >> 8u) % 16ui64) };
			auto world_z{ chunk_z * 16 + static_cast<std::int32_t>((state >> 16u) % 16ui64) };
			auto ground{ owner->surface_height(world_x, world_z) };

			if (lake_biome(owner->biome_for(world_x, world_z, ground)) && ground >= gen_sea_level + 2 && ground <= gen_snow_line - 10)
			{
				lakes.push_back({ chunk_x, chunk_z, world_x, world_z, ground - 1, 4 + static_cast<std::int32_t>((state >> 24u) % 5ui64), 4 + static_cast<std::int32_t>((state >> 32u) % 5ui64), 3 + static_cast<std::int32_t>((state >> 40u) % 4ui64), functions::mix64(state ^ 0x51ED270B7A1CE55Dui64) });
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void features_c::collect_field(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		if (auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 285273191ui64 + static_cast<std::uint64_t>(chunk_z) * 771294323ui64 + owner->get_seed() + 8123ui64) }; state % 100ui64 < 6ui64)
		{
			auto world_x{ chunk_x * 16 + static_cast<std::int32_t>((state >> 8u) % 16ui64) };
			auto world_z{ chunk_z * 16 + static_cast<std::int32_t>((state >> 16u) % 16ui64) };
			auto ground{ owner->surface_height(world_x, world_z) };

			if (auto biome{ owner->biome_for(world_x, world_z, ground) }; (biome == structures::biome_plains || biome == structures::biome_meadow || biome == structures::biome_savanna) && ground >= gen_sea_level + 1 && ground <= gen_snow_line - 20)
			{
				auto radius_x{ 3 + static_cast<std::int32_t>((state >> 24u) % 6ui64) };
				auto radius_z{ 3 + static_cast<std::int32_t>((state >> 32u) % 6ui64) };
				auto flat{ true };

				for (auto probe_z{ -radius_z }; probe_z <= radius_z && flat; probe_z += radius_z)
				{
					for (auto probe_x{ -radius_x }; probe_x <= radius_x && flat; probe_x += radius_x)
					{
						flat = (std::abs(owner->surface_height(world_x + probe_x, world_z + probe_z) - ground) <= 1);
					}
				}

				if (flat)
				{
					fields.push_back({ chunk_x, chunk_z, world_x, world_z, ground, radius_x, radius_z, functions::mix64(state ^ 0x2545F4914F6CDD1Dui64) });
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void features_c::collect_pyramid(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		if (auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 617428711ui64 + static_cast<std::uint64_t>(chunk_z) * 913377029ui64 + owner->get_seed() + 5527ui64) }; state % 100ui64 < 3ui64)
		{
			auto world_x{ chunk_x * 16 + 8 };
			auto world_z{ chunk_z * 16 + 8 };
			auto ground{ owner->surface_height(world_x, world_z) };

			if (owner->biome_for(world_x, world_z, ground) == structures::biome_desert && ground >= gen_sea_level + 2 && ground <= gen_snow_line - 30)
			{
				auto reach{ 10 + static_cast<std::int32_t>((state >> 16u) % 3ui64) };
				auto flat{ true };

				for (auto probe_z{ -reach }; probe_z <= reach && flat; probe_z += reach)
				{
					for (auto probe_x{ -reach }; probe_x <= reach && flat; probe_x += reach)
					{
						auto height{ owner->surface_height(world_x + probe_x, world_z + probe_z) };
						flat = (owner->biome_for(world_x + probe_x, world_z + probe_z, height) == structures::biome_desert && std::abs(height - ground) <= 3);
					}
				}

				if (flat)
				{
					pyramids.push_back({ chunk_x, chunk_z, world_x, world_z, ground, reach, functions::mix64(state ^ 0x7E1D2C3B4A596877ui64) });
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void features_c::carve_lake(const structures::lake_plan_s& plan)
	{
		for (auto offset_z{ -plan.radius_z - 2 }; offset_z <= plan.radius_z + 2; ++offset_z)
		{
			for (auto offset_x{ -plan.radius_x - 2 }; offset_x <= plan.radius_x + 2; ++offset_x)
			{
				auto world_x{ plan.x + offset_x };
				auto world_z{ plan.z + offset_z };

				if (owner->inside(world_x, world_z))
				{
					auto wobble{ static_cast<std::double_t>(functions::mix64(plan.seed + static_cast<std::uint64_t>(offset_x * 7919 + offset_z * 104729)) % 100ui64) / 100.0 };
					auto norm_x{ static_cast<std::double_t>(offset_x) / static_cast<std::double_t>(plan.radius_x) };
					auto norm_z{ static_cast<std::double_t>(offset_z) / static_cast<std::double_t>(plan.radius_z) };

					if (auto falloff{ norm_x * norm_x + norm_z * norm_z + wobble * 0.22 }; falloff <= 1.0)
					{
						auto bowl{ static_cast<std::int32_t>(std::round(static_cast<std::double_t>(plan.depth) * (1.0 - falloff))) };
						auto floor_roll{ functions::mix64(plan.seed + static_cast<std::uint64_t>(offset_x * 31 + offset_z * 131) + 55ui64) };

						for (auto clear{ plan.level + 1 }; clear <= plan.level + 5; ++clear)
						{
							owner->solid(world_x, clear, world_z, structures::block_air);
						}

						for (auto fill{ plan.level }; fill >= plan.level - bowl; --fill)
						{
							owner->solid(world_x, fill, world_z, structures::block_water);
						}

						owner->solid(world_x, plan.level - bowl - 1, world_z, functions::one_in(floor_roll, 5ui64) ? structures::block_clay : (functions::one_in(floor_roll, 3ui64) ? structures::block_gravel : structures::block_sand));

						if (falloff > 0.72)
						{
							owner->solid(world_x, plan.level + 1, world_z, structures::block_air);

							if (functions::one_in(floor_roll, 4ui64))
							{
								owner->solid(world_x, plan.level, world_z, structures::block_sand);
							}
						}
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void features_c::build_field(const structures::field_plan_s& plan)
	{
		auto shape{ static_cast<std::int32_t>(plan.seed % 5ui64) };
		auto channel_kind{ static_cast<std::int32_t>((plan.seed >> 8u) % 3ui64) };
		auto crop_gap{ 8 + static_cast<std::int32_t>((plan.seed >> 16u) % 10ui64) };
		auto corner{ functions::mix64(plan.seed + 991ui64) };

		for (auto offset_z{ -plan.radius_z }; offset_z <= plan.radius_z; ++offset_z)
		{
			for (auto offset_x{ -plan.radius_x }; offset_x <= plan.radius_x; ++offset_x)
			{
				auto world_x{ plan.x + offset_x };
				auto world_z{ plan.z + offset_z };

				if (field_covers(plan, offset_x, offset_z, shape) && owner->inside(world_x, world_z))
				{
					for (auto clear{ plan.ground + 1 }; clear <= plan.ground + 4; ++clear)
					{
						owner->solid(world_x, clear, world_z, structures::block_air);
					}

					if ((channel_kind == 0 && offset_x == 0) || (channel_kind == 1 && offset_z == 0))
					{
						owner->solid(world_x, plan.ground, world_z, structures::block_water);
					}

					else
					{
						owner->solid(world_x, plan.ground, world_z, structures::block_farmland);

						if (static_cast<std::int32_t>(functions::mix64(plan.seed + static_cast<std::uint64_t>(offset_x * 3121 + offset_z * 6247)) % 100ui64) >= crop_gap)
						{
							owner->solid(world_x, plan.ground + 1, world_z, structures::block_wheat);
						}
					}
				}
			}
		}

		if (channel_kind == 2)
		{
			for (auto offset_z{ -1 }; offset_z <= 1; ++offset_z)
			{
				for (auto offset_x{ -1 }; offset_x <= 1; ++offset_x)
				{
					if (owner->inside(plan.x + offset_x, plan.z + offset_z))
					{
						auto rim{ offset_x != 0 || offset_z != 0 };
						owner->solid(plan.x + offset_x, plan.ground, plan.z + offset_z, rim ? structures::block_mossy_cobblestone : structures::block_water);

						if (rim)
						{
							owner->solid(plan.x + offset_x, plan.ground + 1, plan.z + offset_z, structures::block_air);
						}
					}
				}
			}
		}

		if (owner->inside(plan.x + plan.radius_x + 1, plan.z + plan.radius_z + 1))
		{
			for (auto step{ 0 }; step < 1 + static_cast<std::int32_t>(corner % 4ui64); ++step)
			{
				owner->solid(plan.x + plan.radius_x + 1, plan.ground + 1 + step, plan.z + plan.radius_z + 1, structures::block_hay_block);
			}
		}

		if (functions::one_in(corner, 3ui64))
		{
			auto patch_x{ plan.x - plan.radius_x - 2 };
			auto patch_z{ plan.z + static_cast<std::int32_t>((corner >> 8u) % 3ui64) - 1 };

			for (auto step{ 0 }; step < 3; ++step)
			{
				owner->solid(patch_x + step, plan.ground + 1, patch_z, structures::block_pumpkin);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void features_c::build_pyramid(const structures::pyramid_plan_s& plan)
	{
		auto base_y{ plan.ground - 1 };
		auto room{ 3 };
		auto room_y{ base_y + 1 };

		for (auto layer{ 0 }; layer <= plan.reach; ++layer)
		{
			auto span{ plan.reach - layer };
			auto y{ base_y + layer };

			for (auto offset_z{ -span }; offset_z <= span; ++offset_z)
			{
				for (auto offset_x{ -span }; offset_x <= span; ++offset_x)
				{
					if (layer < 1 || std::abs(offset_x) == span || std::abs(offset_z) == span)
					{
						auto roll{ functions::mix64(plan.seed + static_cast<std::uint64_t>((plan.x + offset_x) * 31 + (plan.z + offset_z) * 131 + y * 17)) };
						owner->solid(plan.x + offset_x, y, plan.z + offset_z, functions::one_in(roll, 23ui64) ? structures::block_orange_terracotta : (functions::one_in(roll, 37ui64) ? structures::block_white_terracotta : structures::block_sandstone));
					}
				}
			}
		}

		for (auto layer{ 0 }; layer < plan.reach; ++layer)
		{
			auto span{ plan.reach - layer };

			for (auto offset_z{ -span + 1 }; offset_z <= span - 1; ++offset_z)
			{
				for (auto offset_x{ -span + 1 }; offset_x <= span - 1; ++offset_x)
				{
					owner->solid(plan.x + offset_x, base_y + layer, plan.z + offset_z, structures::block_sandstone);
				}
			}
		}

		for (auto y{ room_y }; y <= room_y + 3; ++y)
		{
			for (auto offset_z{ -room }; offset_z <= room; ++offset_z)
			{
				for (auto offset_x{ -room }; offset_x <= room; ++offset_x)
				{
					owner->solid(plan.x + offset_x, y, plan.z + offset_z, (std::abs(offset_x) == room || std::abs(offset_z) == room || y == room_y + 3) ? structures::block_orange_terracotta : structures::block_air);
				}
			}
		}

		for (auto offset_z{ -room + 1 }; offset_z <= room - 1; ++offset_z)
		{
			for (auto offset_x{ -room + 1 }; offset_x <= room - 1; ++offset_x)
			{
				owner->solid(plan.x + offset_x, room_y - 1, plan.z + offset_z, structures::block_white_terracotta);
			}
		}

		for (auto step{ 0 }; step <= plan.reach; ++step)
		{
			owner->solid(plan.x, room_y + 1, plan.z - room - step, structures::block_air);
		}
	}
	/*
	//=====================================================================================
	*/
	bool features_c::lake_biome(structures::biome_e biome)
	{
		return (biome != structures::biome_ocean && biome != structures::biome_beach && biome != structures::biome_desert && biome != structures::biome_badlands && biome != structures::biome_stony_peaks && biome != structures::biome_jagged_peaks);
	}
	/*
	//=====================================================================================
	*/
	bool features_c::field_covers(const structures::field_plan_s& plan, std::int32_t offset_x, std::int32_t offset_z, std::int32_t shape)
	{
		auto covers{ std::abs(offset_x) + std::abs(offset_z) <= plan.radius_x + plan.radius_z / 2 };

		switch (shape)
		{
		case 0:
			covers = true;
			break;

		case 1:
			covers = (ellipse(offset_x, offset_z, plan.radius_x + 1, plan.radius_z + 1) <= 1.0);
			break;

		case 2:
			covers = (offset_x >= 0 || offset_z >= 0);
			break;

		case 3:
			covers = (std::abs(offset_x) != plan.radius_x / 2 && std::abs(offset_z) != plan.radius_z / 2);
			break;
		}

		return covers;
	}
	/*
	//=====================================================================================
	*/
	bool features_c::is_near(std::int32_t from_x, std::int32_t from_z, std::int32_t to_x, std::int32_t to_z)
	{
		return (std::abs(from_x - to_x) <= 1 && std::abs(from_z - to_z) <= 1);
	}
	/*
	//=====================================================================================
	*/
	std::double_t features_c::ellipse(std::int32_t offset_x, std::int32_t offset_z, std::int32_t radius_x, std::int32_t radius_z)
	{
		auto norm_x{ static_cast<std::double_t>(offset_x) / static_cast<std::double_t>(radius_x) };
		auto norm_z{ static_cast<std::double_t>(offset_z) / static_cast<std::double_t>(radius_z) };

		return norm_x * norm_x + norm_z * norm_z;
	}
}

//=====================================================================================
