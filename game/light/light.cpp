
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

		auto count{ static_cast<std::uint32_t>(top + 1) };

		if (count)
		{
			load_filters(chunk_x, chunk_z, count * 16u);
			spread(count * 16u);
			nibbles.assign(static_cast<std::size_t>(count) * light_array_size, 0u);

			for (auto section{ 0u }; section < count; ++section)
			{
				for (auto index{ 0u }; index < section_volume; ++index)
				{
					auto cell{ ((section * 16u + (index >> 8u)) * light_region + ((index >> 4u) & 15u) + light_border) * light_region + (index & 15u) + light_border };
					nibbles[section * light_array_size + index / 2u] |= static_cast<std::uint8_t>(levels[cell] << ((index & 1u) * 4u));
				}
			}
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
	void light_c::load_filters(std::int32_t chunk_x, std::int32_t chunk_z, std::uint32_t height)
	{
		filters.assign(static_cast<std::size_t>(height) * light_region * light_region, 0u);

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
								auto flags{ terrain.get_state(terrain.get_block(*section, block)).flags };
								auto cell{ ((index * 16u + (block >> 8u)) * light_region + static_cast<std::uint32_t>(region_z)) * light_region + static_cast<std::uint32_t>(region_x) };
								filters[cell] = (flags & terrain_flag_solid) ? light_level_max : ((flags & terrain_flag_dim) ? 1u : 0u);
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

		for (auto next{ 0u }; next < queue.size(); ++next)
		{
			auto cell{ queue[next] };
			auto x{ cell % light_region };
			auto z{ (cell / light_region) % light_region };
			auto y{ cell / (light_region * light_region) };
			std::array<std::uint32_t, 6u> neighbours{ x > 0u ? cell - 1u : cell, x + 1u < light_region ? cell + 1u : cell, z > 0u ? cell - light_region : cell, z + 1u < light_region ? cell + light_region : cell, y > 0u ? cell - light_region * light_region : cell, y + 1u < height ? cell + light_region * light_region : cell };

			for (auto neighbour : neighbours)
			{
				if (auto level{ levels[cell] }; neighbour != cell && filters[neighbour] < light_level_max && level > 1u + filters[neighbour] && level - 1u - filters[neighbour] > levels[neighbour])
				{
					levels[neighbour] = static_cast<std::uint8_t>(level - 1u - filters[neighbour]);
					queue.push_back(neighbour);
				}
			}
		}
	}
}

//=====================================================================================
