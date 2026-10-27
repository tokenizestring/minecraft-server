
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	void caverns_c::dress(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		owner = parent;

		auto state{ functions::mix64(static_cast<std::uint64_t>(chunk_x) * 733271741ui64 + static_cast<std::uint64_t>(chunk_z) * 199933717ui64 + owner->get_seed() + 3307ui64) };
		auto lush{ functions::one_in(functions::mix64(state + 11ui64), 5ui64) };
		auto dripping{ functions::one_in(functions::mix64(state + 23ui64), 4ui64) };

		if (functions::one_in(functions::mix64(state + 37ui64), 22ui64))
		{
			auto spot{ functions::mix64(state + 41ui64) };
			auto geode_x{ 4 + static_cast<std::int32_t>(spot % 8ui64) };
			auto geode_z{ 4 + static_cast<std::int32_t>((spot >> 8u) % 8ui64) };
			auto geode_y{ -50 + static_cast<std::int32_t>((spot >> 16u) % 40ui64) };

			if (cave_solid(owner->get_block(geode_x, geode_y, geode_z)))
			{
				geode(geode_x, geode_y, geode_z, spot);
			}
		}

		for (auto local_z{ 0 }; local_z < 16; ++local_z)
		{
			for (auto local_x{ 0 }; local_x < 16; ++local_x)
			{
				for (auto y{ gen_dressing_floor }; y < gen_dressing_ceiling; ++y)
				{
					if (owner->get_block(local_x, y, local_z) == structures::block_air)
					{
						state = functions::mix64(state + gen_golden);
						dress_cell(local_x, y, local_z, state, lush, dripping);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::dress_cell(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t state, bool lush, bool dripping)
	{
		auto roll{ static_cast<std::uint32_t>(state % 1000ui64) };

		if (roll < 620u && cave_solid(owner->get_block(local_x, y + 1, local_z)))
		{
			if (dripping && roll < 150u)
			{
				hang_dripstone(local_x, y, local_z, 1 + static_cast<std::int32_t>((state >> 12u) % 4ui64));
			}

			else if (lush && roll < 250u)
			{
				hang_cave_vine(local_x, y, local_z, 2 + static_cast<std::int32_t>((state >> 16u) % 5ui64));
			}

			else if (roll < 300u)
			{
				owner->set_block(local_x, y, local_z, structures::block_glow_lichen_down);
			}

			else if (roll < 330u && y < -30)
			{
				owner->set_block(local_x, y, local_z, structures::block_cobweb);
			}

			else
			{
				rough_ceiling(local_x, y, local_z, state + 8821ui64);
			}
		}

		else if (cave_solid(owner->get_block(local_x, y - 1, local_z)))
		{
			if (roll >= 90u && roll < 250u)
			{
				rough_stone(local_x, y, local_z, state + 5501ui64);
			}

			else if (dripping && roll < 22u)
			{
				raise_dripstone(local_x, y, local_z, 1 + static_cast<std::int32_t>((state >> 20u) % 3ui64));
			}

			else if (lush && roll < 70u)
			{
				owner->set_block(local_x, y - 1, local_z, structures::block_moss_block);

				if (roll < 40u)
				{
					owner->set_block(local_x, y, local_z, structures::block_moss_carpet);
				}
			}

			else if (roll < 82u)
			{
				owner->set_block(local_x, y, local_z, roll < 76u ? structures::block_brown_mushroom : structures::block_red_mushroom);
			}

			else if (y < -42 && roll < 90u)
			{
				owner->set_block(local_x, y - 1, local_z, structures::block_sculk);

				if (roll < 84u)
				{
					owner->set_block(local_x, y, local_z, structures::block_sculk_vein_down);
				}
			}

			else if (roll < 84u && local_x > 2 && local_x < 13 && local_z > 2 && local_z < 13)
			{
				cave_pool(local_x, y, local_z, y < -45 ? structures::block_lava : structures::block_water, state);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::hang_dripstone(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::int32_t length)
	{
		for (auto step{ 0 }; step < length && y - step > world_min_y && owner->get_block(local_x, y - step, local_z) == structures::block_air; ++step)
		{
			owner->set_block(local_x, y - step, local_z, step == length - 1 ? structures::block_dripstone_tip_down : ((step < 1 && length > 2) ? structures::block_dripstone_base_down : structures::block_dripstone_frustum_down));
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::raise_dripstone(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::int32_t length)
	{
		for (auto step{ 0 }; step < length && y + step < world_min_y + world_height && owner->get_block(local_x, y + step, local_z) == structures::block_air; ++step)
		{
			owner->set_block(local_x, y + step, local_z, step == length - 1 ? structures::block_dripstone_tip_up : ((step < 1 && length > 2) ? structures::block_dripstone_base_up : structures::block_dripstone_frustum_up));
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::hang_cave_vine(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::int32_t length)
	{
		for (auto step{ 0 }; step < length && y - step > world_min_y && owner->get_block(local_x, y - step, local_z) == structures::block_air; ++step)
		{
			owner->set_block(local_x, y - step, local_z, step == length - 1 ? structures::block_cave_vines_berries : structures::block_cave_vines_plant);
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::cave_pool(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint8_t liquid, std::uint64_t seed)
	{
		for (auto offset_z{ -2 }; offset_z <= 2; ++offset_z)
		{
			for (auto offset_x{ -2 }; offset_x <= 2; ++offset_x)
			{
				auto probe_x{ local_x + offset_x };
				auto probe_z{ local_z + offset_z };

				if (offset_x * offset_x + offset_z * offset_z <= 4 && probe_x >= 0 && probe_x <= 15 && probe_z >= 0 && probe_z <= 15 && owner->get_block(probe_x, y, probe_z) == structures::block_air && cave_solid(owner->get_block(probe_x, y - 1, probe_z)))
				{
					owner->set_block(probe_x, y, probe_z, liquid);

					if (functions::one_in(functions::mix64(seed + static_cast<std::uint64_t>(offset_x * 31 + offset_z * 71)), 4ui64))
					{
						owner->set_block(probe_x, y - 1, probe_z, liquid == structures::block_water ? structures::block_clay : structures::block_magma_block);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::geode(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t seed)
	{
		auto radius{ 4 + static_cast<std::int32_t>(seed % 2ui64) };

		for (auto offset_y{ -radius }; offset_y <= radius; ++offset_y)
		{
			for (auto offset_z{ -radius }; offset_z <= radius; ++offset_z)
			{
				for (auto offset_x{ -radius }; offset_x <= radius; ++offset_x)
				{
					auto probe_x{ local_x + offset_x };
					auto probe_y{ y + offset_y };
					auto probe_z{ local_z + offset_z };
					auto squared{ offset_x * offset_x + offset_y * offset_y + offset_z * offset_z };

					if (probe_x >= 0 && probe_x <= 15 && probe_z >= 0 && probe_z <= 15 && probe_y > world_min_y + 2 && probe_y < world_min_y + world_height && squared <= radius * radius)
					{
						auto block{ structures::block_air };

						if (squared > (radius - 1) * (radius - 1))
						{
							block = structures::block_smooth_basalt;
						}

						else if (squared > (radius - 2) * (radius - 2))
						{
							block = structures::block_calcite;
						}

						else if (squared > (radius - 3) * (radius - 3))
						{
							block = functions::one_in(functions::mix64(seed + static_cast<std::uint64_t>(squared)), 7ui64) ? structures::block_budding_amethyst : structures::block_amethyst_block;
						}

						owner->set_block(probe_x, probe_y, probe_z, block);
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::rough_stone(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t seed)
	{
		if (auto ground{ owner->get_block(local_x, y - 1, local_z) }; cave_solid(ground))
		{
			auto roll{ static_cast<std::uint32_t>(functions::mix64(seed) % 1000ui64) };
			auto side{ static_cast<std::size_t>(functions::mix64(seed + 31ui64) % 4ui64) };
			auto probe_x{ local_x + gen_side_x[side] };
			auto probe_z{ local_z + gen_side_z[side] };

			if (roll < 300u)
			{
				owner->set_block(local_x, y, local_z, floor_slab_for(ground));
			}

			else if (roll < 520u)
			{
				owner->set_block(local_x, y - 1, local_z, ground == structures::block_deepslate ? structures::block_cobbled_deepslate : structures::block_cobblestone);
			}

			else if (roll < 640u && probe_x >= 0 && probe_x <= 15 && probe_z >= 0 && probe_z <= 15 && cave_solid(owner->get_block(probe_x, y, probe_z)))
			{
				auto cobble{ ground == structures::block_deepslate || functions::one_in(functions::mix64(seed + 77ui64), 2ui64) };
				owner->set_block(local_x, y, local_z, static_cast<std::uint8_t>((cobble ? structures::block_cobblestone_stairs_north : structures::block_stone_stairs_north) + gen_stair_turns[side]));
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void caverns_c::rough_ceiling(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t seed)
	{
		if (auto roof{ owner->get_block(local_x, y + 1, local_z) }; cave_solid(roof))
		{
			auto roll{ static_cast<std::uint32_t>(functions::mix64(seed) % 1000ui64) };

			if (roll < 300u)
			{
				owner->set_block(local_x, y, local_z, ceiling_slab_for(roof));
			}

			else if (roll < 560u)
			{
				owner->set_block(local_x, y + 1, local_z, roof == structures::block_deepslate ? structures::block_cobbled_deepslate : structures::block_cobblestone);
			}

			else if (roll < 700u)
			{
				owner->set_block(local_x, y, local_z, structures::block_hanging_roots);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	bool caverns_c::cave_solid(std::uint8_t block)
	{
		return (block == structures::block_stone || block == structures::block_deepslate || block == structures::block_granite || block == structures::block_diorite || block == structures::block_andesite || block == structures::block_tuff || block == structures::block_calcite || block == structures::block_dripstone_block || block == structures::block_smooth_basalt);
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t caverns_c::floor_slab_for(std::uint8_t ground)
	{
		auto slab{ structures::block_stone_slab };

		if (ground == structures::block_deepslate)
		{
			slab = structures::block_cobbled_deepslate_slab;
		}

		else if (ground == structures::block_moss_block)
		{
			slab = structures::block_mossy_cobblestone_slab;
		}

		else if (ground == structures::block_cobblestone)
		{
			slab = structures::block_cobblestone_slab;
		}

		return slab;
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t caverns_c::ceiling_slab_for(std::uint8_t roof)
	{
		auto slab{ structures::block_stone_slab_top };

		if (roof == structures::block_deepslate)
		{
			slab = structures::block_cobbled_deepslate_slab_top;
		}

		else if (roof == structures::block_moss_block)
		{
			slab = structures::block_mossy_cobblestone_slab_top;
		}

		else if (roof == structures::block_cobblestone)
		{
			slab = structures::block_cobblestone_slab_top;
		}

		return slab;
	}
}

//=====================================================================================
