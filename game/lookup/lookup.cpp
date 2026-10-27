
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	lookup_c lookup;

	std::uint16_t lookup_c::get(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z)
	{
		auto kind{ edits.find(target, x, y, z) };
		auto inside{ y >= world_min_y && y < world_min_y + world_height };

		if (kind == kind_none && inside && target == structures::world_smp)
		{
			kind = get_column(x >> 4, z >> 4)[static_cast<std::size_t>(((y - world_min_y) * 16 + (z & 15)) * 16 + (x & 15))];
		}

		return kind;
	}
	/*
	//=====================================================================================
	*/
	bool lookup_c::is_land(std::int32_t x, std::int32_t z)
	{
		sync();
		return generator.surface_height(x, z) > gen_sea_level + 1;
	}
	/*
	//=====================================================================================
	*/
	void lookup_c::sync()
	{
		if (seed != config.smp_seed)
		{
			generator.set_seed(config.smp_seed);
			seed = config.smp_seed;
			columns.clear();
		}
	}
	/*
	//=====================================================================================
	*/
	std::int32_t lookup_c::get_top(std::int32_t x, std::int32_t z, std::uint16_t& kind)
	{
		auto top{ world_min_y - 1 };
		kind = kind_none;

		for (auto y{ world_min_y + world_height - 1 }; top < world_min_y && y >= world_min_y; --y)
		{
			auto found{ get(structures::world_smp, x, y, z) };
			top = found != structures::block_air ? y : top;
			kind = found != structures::block_air ? found : kind;
		}

		return top;
	}
	/*
	//=====================================================================================
	*/
	const std::array<std::uint8_t, column_volume>& lookup_c::get_column(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto key{ edits_c::get_key(chunk_x, chunk_z) };

		sync();

		if (columns.size() >= lookup_cache_chunks && columns.find(key) == columns.end())
		{
			columns.erase(std::min_element(columns.begin(), columns.end(), [](const auto& a, const auto& b) { return a.second.last_use < b.second.last_use; }));
		}

		auto& column{ columns[key] };
		clock += 1u;
		column.last_use = clock;

		if (column.blocks == nullptr)
		{
			generator.generate(chunk_x, chunk_z);
			column.blocks = std::make_unique<std::array<std::uint8_t, column_volume>>();
			std::copy(generator.get_blocks(), generator.get_blocks() + column_volume, column.blocks->begin());
		}

		return *column.blocks;
	}
}

//=====================================================================================
