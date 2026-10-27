
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_glow_c java_glow;

	void java_glow_c::tick()
	{
		for (auto player : world.get_players())
		{
			auto* client{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };

			if (client && functions::is_modern(client->family))
			{
				update(*client);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void java_glow_c::clear(structures::client_s& client)
	{
		if (client.glow_level)
		{
			broadcast(client, client.glow_x, client.glow_y, client.glow_z, 0u);
			client.glow_level = 0u;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_glow_c::update(structures::client_s& client)
	{
		auto level{ config.held_light && client.player.world == structures::world_smp ? get_level(client.player) : 0u };
		auto x{ static_cast<std::int32_t>(std::floor(client.player.x)) };
		auto y{ static_cast<std::int32_t>(std::floor(client.player.y)) + held_light_offset };
		auto z{ static_cast<std::int32_t>(std::floor(client.player.z)) };
		auto wanted{ static_cast<std::uint8_t>(level && is_open(client.player.world, x, y, z) ? level : 0u) };
		auto moved{ x != client.glow_x || y != client.glow_y || z != client.glow_z };

		if (wanted != client.glow_level || (wanted && moved))
		{
			clear(client);

			if (wanted)
			{
				broadcast(client, x, y, z, wanted);
			}

			client.glow_level = wanted;
			client.glow_x = x;
			client.glow_y = y;
			client.glow_z = z;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_glow_c::broadcast(const structures::client_s& owner, std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t level)
	{
		auto radius{ static_cast<std::int32_t>(config.view_distance) };
		auto target{ owner.player.world };
		auto kind{ target == structures::world_smp ? lookup.get(target, x, y, z) : static_cast<std::uint16_t>(structures::block_air) };

		for (auto player : world.get_players())
		{
			auto* viewer{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };

			if (viewer && functions::is_modern(viewer->family) && player->world == target && std::abs((x >> 4) - viewer->view_x) <= radius && std::abs((z >> 4) - viewer->view_z) <= radius)
			{
				packet.clear();
				functions::write_varint(packet, viewer->layout.play_out_block_update);
				functions::write_u64(packet, functions::encode_position(x, y, z));
				functions::write_varint(packet, static_cast<std::int32_t>(level ? blocks.get_light_state(viewer->family, level) : blocks.get_state(viewer->family, kind == kind_none ? static_cast<std::uint16_t>(structures::block_air) : kind)));
				network.send(*viewer, packet);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t java_glow_c::get_level(const structures::player_s& player)
	{
		const auto& held{ player.inventory[player.held_slot] };
		const auto& offhand{ player.inventory[offhand_slot] };
		auto main_level{ held.count ? blocks.get_emission(items.get_block(held.item)) : 0u };
		auto off_level{ offhand.count ? blocks.get_emission(items.get_block(offhand.item)) : 0u };
		return static_cast<std::uint8_t>(std::max(main_level, off_level));
	}
	/*
	//=====================================================================================
	*/
	bool java_glow_c::is_open(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z)
	{
		return target == structures::world_smp ? lookup.get(target, x, y, z) == structures::block_air : terrain.is_air_at(x, y, z);
	}
}

//=====================================================================================
