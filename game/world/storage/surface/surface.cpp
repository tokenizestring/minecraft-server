
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	surface_c surface;

	void surface_c::store(std::int32_t chunk_x, std::int32_t chunk_z, std::shared_ptr<const std::array<std::int16_t, 256u>> map)
	{
		maps[edits_c::get_key(chunk_x, chunk_z)] = std::move(map);

		if (maps.size() > surface_cap)
		{
			trim();
		}
	}
	/*
	//=====================================================================================
	*/
	std::int16_t surface_c::get(std::int32_t x, std::int32_t z) const
	{
		auto found{ maps.find(edits_c::get_key(x >> 4, z >> 4)) };
		return found != maps.end() ? (*found->second)[static_cast<std::size_t>(((z & 15) << 4) | (x & 15))] : surface_none;
	}
	/*
	//=====================================================================================
	*/
	bool surface_c::has(std::int32_t chunk_x, std::int32_t chunk_z) const
	{
		return maps.contains(edits_c::get_key(chunk_x, chunk_z));
	}
	/*
	//=====================================================================================
	*/
	void surface_c::change(std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind)
	{
		auto found{ maps.find(edits_c::get_key(x >> 4, z >> 4)) };

		if (found != maps.end())
		{
			auto map{ std::make_shared<std::array<std::int16_t, 256u>>(*found->second) };
			auto& stand{ (*map)[static_cast<std::size_t>(((z & 15) << 4) | (x & 15))] };
			auto solid{ kind != structures::block_air && kind != kind_none && (kind >= placed_kind_base || blocks.is_fluid(static_cast<std::uint8_t>(kind)) == false) };
			auto fluid{ kind < placed_kind_base && kind != structures::block_air && blocks.is_fluid(static_cast<std::uint8_t>(kind)) };

			auto open{ stand != surface_none };

			if (open && solid && y == stand + 1)
			{
				stand = static_cast<std::int16_t>(y);
			}

			else if (open && (solid || fluid) && y > stand && y <= stand + 2)
			{
				stand = surface_none;
			}

			else if (open && kind == structures::block_air && y == stand)
			{
				std::uint16_t below{ structures::block_air };
				auto floor{ y - 1 };

				for (; floor > y - surface_rescan && floor > world_min_y && below == structures::block_air; --floor)
				{
					below = lookup.get(structures::world_smp, x, floor, z);
				}

				auto walkable{ below != structures::block_air && below != kind_none && (below >= placed_kind_base || blocks.is_fluid(static_cast<std::uint8_t>(below)) == false) };
				stand = walkable ? static_cast<std::int16_t>(floor + 1) : surface_none;
			}

			found->second = std::move(map);
		}
	}
	/*
	//=====================================================================================
	*/
	void surface_c::clear()
	{
		maps.clear();
	}
	/*
	//=====================================================================================
	*/
	void surface_c::trim()
	{
		std::erase_if(maps, [](const auto& entry)
		{
			auto chunk_x{ static_cast<std::int32_t>(entry.first >> 32u) };
			auto chunk_z{ static_cast<std::int32_t>(entry.first & 0xFFFFFFFFu) };
			auto close{ false };

			for (auto player : world.get_players())
			{
				close = close || (player->world == structures::world_smp && std::abs((static_cast<std::int32_t>(std::floor(player->x)) >> 4) - chunk_x) <= mob_despawn_chunks && std::abs((static_cast<std::int32_t>(std::floor(player->z)) >> 4) - chunk_z) <= mob_despawn_chunks);
			}

			return close == false;
		});
	}
}

//=====================================================================================
