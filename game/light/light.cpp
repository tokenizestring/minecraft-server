
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	light_c light;

	std::uint32_t light_c::compute_sky(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto top{ -1 };

		for (auto x{ -1 }; x <= 1; ++x)
		{
			for (auto z{ -1 }; z <= 1; ++z)
			{
				for (auto index{ static_cast<std::int32_t>(modern_section_count) - 1 }; index > top; --index)
				{
					if (terrain.get_section(chunk_x + x, modern_min_section + index, chunk_z + z))
					{
						top = index;
					}
				}
			}
		}

		auto count{ top >= 0 ? std::min(static_cast<std::uint32_t>(top + 2), modern_section_count) : 0u };

		if (count)
		{
			load_filters(chunk_x, chunk_z, count * 16u);
			spread(count * 16u);
			pack(count);
		}

		return count;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t light_c::compute_generated(const std::uint8_t* column, std::int32_t highest, const around_t& around)
	{
		auto count{ highest >= world_min_y ? std::min(static_cast<std::uint32_t>(((highest - world_min_y) >> 4) + 2), modern_section_count) : 0u };

		if (count)
		{
			filters.resize(static_cast<std::size_t>(count) * 16u * light_region * light_region);
			emits.assign(filters.size(), 0u);

			for (auto y{ 0u }; y < count * 16u; ++y)
			{
				for (auto z{ 0u }; z < light_region; ++z)
				{
					auto source_z{ std::clamp(static_cast<std::int32_t>(z) - static_cast<std::int32_t>(light_border), 0, 15) };
					auto inside_z{ z >= light_border && z < light_border + 16u };

					for (auto x{ 0u }; x < light_region; ++x)
					{
						auto source_x{ std::clamp(static_cast<std::int32_t>(x) - static_cast<std::int32_t>(light_border), 0, 15) };
						auto block{ column[(y * 16u + static_cast<std::uint32_t>(source_z)) * 16u + static_cast<std::uint32_t>(source_x)] };
						auto cell{ (y * light_region + z) * light_region + x };
						filters[cell] = blocks.get_filter(block);
						emits[cell] = inside_z && x >= light_border && x < light_border + 16u ? blocks.get_emission(block) : 0u;
					}
				}
			}

			load_edits(around, count * 16u);
			spread(count * 16u);
			pack(count);
		}

		return count;
	}
	/*
	//=====================================================================================
	*/
	const std::uint8_t* light_c::get_section(std::uint32_t section) const
	{
		return nibbles.data() + static_cast<std::size_t>(section) * light_array_size;
	}
	/*
	//=====================================================================================
	*/
	const std::uint8_t* light_c::get_block_section(std::uint32_t section) const
	{
		return block_nibbles.data() + static_cast<std::size_t>(section) * light_array_size;
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t light_c::get_block_mask() const
	{
		return block_mask;
	}
	/*
	//=====================================================================================
	*/
	void light_c::pack(std::uint32_t count)
	{
		nibbles.assign(static_cast<std::size_t>(count) * light_array_size, 0u);
		block_nibbles.assign(static_cast<std::size_t>(count) * light_array_size, 0u);
		block_mask = 0u;

		for (auto section{ 0u }; section < count; ++section)
		{
			for (auto index{ 0u }; index < section_volume; ++index)
			{
				auto cell{ ((section * 16u + (index >> 8u)) * light_region + ((index >> 4u) & 15u) + light_border) * light_region + (index & 15u) + light_border };
				auto shift{ (index & 1u) * 4u };
				nibbles[section * light_array_size + index / 2u] |= static_cast<std::uint8_t>(levels[cell] << shift);
				block_nibbles[section * light_array_size + index / 2u] |= static_cast<std::uint8_t>(block_levels[cell] << shift);
				block_mask |= block_levels[cell] ? 1ui64 << section : 0ui64;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void light_c::load_filters(std::int32_t chunk_x, std::int32_t chunk_z, std::uint32_t height)
	{
		filters.assign(static_cast<std::size_t>(height) * light_region * light_region, 0u);
		emits.assign(filters.size(), 0u);

		for (auto x{ -1 }; x <= 1; ++x)
		{
			for (auto z{ -1 }; z <= 1; ++z)
			{
				for (auto index{ 0u }; index < height / 16u; ++index)
				{
					if (const auto* section{ terrain.get_section(chunk_x + x, modern_min_section + static_cast<std::int32_t>(index), chunk_z + z) }; section)
					{
						for (auto block{ 0u }; block < section_volume; ++block)
						{
							auto region_x{ (x + 1) * 16 + static_cast<std::int32_t>(block & 15u) - static_cast<std::int32_t>(16u - light_border) };
							auto region_z{ (z + 1) * 16 + static_cast<std::int32_t>((block >> 4u) & 15u) - static_cast<std::int32_t>(16u - light_border) };

							if (region_x >= 0 && region_x < static_cast<std::int32_t>(light_region) && region_z >= 0 && region_z < static_cast<std::int32_t>(light_region))
							{
								const auto& state{ terrain.get_state(terrain.get_block(*section, block)) };
								auto cell{ ((index * 16u + (block >> 8u)) * light_region + static_cast<std::uint32_t>(region_z)) * light_region + static_cast<std::uint32_t>(region_x) };
								filters[cell] = (state.flags & terrain_flag_solid) ? light_level_max : ((state.flags & terrain_flag_dim) ? 1u : 0u);
								emits[cell] = blocks.get_state_emission(state.java_26_3);
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
	void light_c::load_edits(const around_t& around, std::uint32_t height)
	{
		for (auto chunk{ 0u }; chunk < around.size(); ++chunk)
		{
			for (const auto& edit : around[chunk] ? *around[chunk] : no_edits)
			{
				auto region_x{ static_cast<std::int32_t>((chunk % 3u) * 16u + (edit.index & 15u)) - static_cast<std::int32_t>(16u - light_border) };
				auto region_z{ static_cast<std::int32_t>((chunk / 3u) * 16u + ((edit.index >> 4u) & 15u)) - static_cast<std::int32_t>(16u - light_border) };
				auto y{ edit.index >> 8u };

				if (region_x >= 0 && region_x < static_cast<std::int32_t>(light_region) && region_z >= 0 && region_z < static_cast<std::int32_t>(light_region) && y < height)
				{
					auto cell{ (y * light_region + static_cast<std::uint32_t>(region_z)) * light_region + static_cast<std::uint32_t>(region_x) };
					filters[cell] = chunk == 4u ? blocks.get_opacity(edit.kind) : filters[cell];
					emits[cell] = chunk == 4u ? blocks.get_emission(edit.kind) : std::max(emits[cell], blocks.get_emission(edit.kind));
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void light_c::spread(std::uint32_t height)
	{
		levels.assign(filters.size(), 0u);
		queue.clear();

		for (auto column{ 0u }; column < light_region * light_region; ++column)
		{
			auto level{ light_level_max };

			for (auto y{ static_cast<std::int32_t>(height) - 1 }; y >= 0 && level; --y)
			{
				auto cell{ static_cast<std::uint32_t>(y) * light_region * light_region + column };
				level = level > filters[cell] ? level - filters[cell] : 0u;
				levels[cell] = static_cast<std::uint8_t>(level);

				if (level > 1u)
				{
					queue.push_back(cell);
				}
			}
		}

		flood(levels, height);

		block_levels.assign(filters.size(), 0u);
		queue.clear();

		for (auto cell{ 0u }; cell < emits.size(); ++cell)
		{
			if (emits[cell])
			{
				block_levels[cell] = emits[cell];
				queue.push_back(cell);
			}
		}

		flood(block_levels, height);
	}
	/*
	//=====================================================================================
	*/
	void light_c::flood(std::vector<std::uint8_t>& target, std::uint32_t height)
	{
		for (auto next{ 0u }; next < queue.size(); ++next)
		{
			auto cell{ queue[next] };
			auto x{ cell % light_region };
			auto z{ (cell / light_region) % light_region };
			auto y{ cell / (light_region * light_region) };
			std::array<std::uint32_t, 6u> neighbours{ x > 0u ? cell - 1u : cell, x + 1u < light_region ? cell + 1u : cell, z > 0u ? cell - light_region : cell, z + 1u < light_region ? cell + light_region : cell, y > 0u ? cell - light_region * light_region : cell, y + 1u < height ? cell + light_region * light_region : cell };

			for (auto neighbour : neighbours)
			{
				if (auto level{ target[cell] }; neighbour != cell && filters[neighbour] < light_level_max && level > 1u + filters[neighbour] && level - 1u - filters[neighbour] > target[neighbour])
				{
					target[neighbour] = static_cast<std::uint8_t>(level - 1u - filters[neighbour]);
					queue.push_back(neighbour);
				}
			}
		}
	}
}

//=====================================================================================
