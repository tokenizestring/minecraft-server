
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	edits_c edits;

	bool edits_c::set(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind)
	{
		auto inside{ y >= world_min_y && y < world_min_y + world_height };

		if (inside)
		{
			auto& slot{ chunks[target][get_key(x >> 4, z >> 4)] };
			auto list{ slot ? std::make_shared<std::vector<structures::edit_s>>(*slot) : std::make_shared<std::vector<structures::edit_s>>() };
			auto index{ static_cast<std::uint32_t>(((y - world_min_y) * 16 + (z & 15)) * 16 + (x & 15)) };
			auto found{ std::find_if(list->begin(), list->end(), [index](const auto& edit) { return edit.index == index; }) };

			if (found != list->end())
			{
				found->kind = kind;
			}

			else
			{
				list->push_back(structures::edit_s{ index, kind });
			}

			slot = std::move(list);
		}

		return inside;
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<structures::edit_s>> edits_c::get(structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z) const
	{
		auto found{ chunks[target].find(get_key(chunk_x, chunk_z)) };
		return found != chunks[target].end() ? found->second : nullptr;
	}
	/*
	//=====================================================================================
	*/
	void edits_c::clear(structures::world_e target)
	{
		chunks[target].clear();
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t edits_c::get_key(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_x)) << 32u) | static_cast<std::uint32_t>(chunk_z);
	}
}

//=====================================================================================
