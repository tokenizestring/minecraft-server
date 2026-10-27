
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_block_c java_block;

	void java_block_c::handle_modern(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		auto sequence{ functions::read_trailing_varint(in) };
		auto smp{ client.player.world == structures::world_smp };

		if (id == client.layout.play_in_use_item)
		{
			functions::read_varint(in);
			sequence = functions::read_varint(in);
		}

		else if (id == client.layout.play_in_player_action)
		{
			auto status{ functions::read_varint(in) };
			auto position{ functions::decode_position(functions::read_u64(in)) };
			functions::read_u8(in);
			sequence = functions::read_varint(in);

			if (in.good())
			{
				dig(client, status, position[0], position[1], position[2]);
			}
		}

		else if (id == client.layout.play_in_use_item_on)
		{
			auto hand{ functions::read_varint(in) };
			auto position{ functions::decode_position(functions::read_u64(in)) };
			auto face{ static_cast<std::uint32_t>(functions::read_varint(in)) };

			if (in.good() && smp && hand == 0 && face < std::size(face_offsets))
			{
				place(client, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2]);
			}
		}

		if (in.good())
		{
			packet.clear();
			functions::write_varint(packet, client.layout.play_out_block_changed_ack);
			functions::write_varint(packet, sequence);
			network.send(client, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::handle_dig_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto status{ static_cast<std::int32_t>(functions::read_u8(in)) };
		auto position{ functions::decode_position_1_8(functions::read_u64(in)) };

		if (in.good())
		{
			dig(client, status, position[0], position[1], position[2]);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::handle_place_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto position{ functions::decode_position_1_8(functions::read_u64(in)) };
		auto face{ functions::read_u8(in) };
		auto smp{ client.player.world == structures::world_smp };
		auto valid{ in.good() && face < std::size(face_offsets) };

		if (valid && smp)
		{
			place(client, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2]);
		}

		else if (valid)
		{
			restore_1_8(client, position[0], position[1], position[2]);
			restore_1_8(client, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2]);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::dig(structures::client_s& client, std::int32_t status, std::int32_t x, std::int32_t y, std::int32_t z)
	{
		auto smp{ client.player.world == structures::world_smp };
		auto survival{ client.player.gamemode != gamemode_creative };

		if (status == drop_all_status || status == drop_one_status)
		{
			java_inventory.drop(client, status == drop_all_status);
		}

		else if (smp && can_break(client, status) && survival)
		{
			auto drop{ items.get_drop(lookup.get(structures::world_smp, x, y, z)) };
			change(structures::world_smp, x, y, z, structures::block_air);

			if (drop && drops.spawn(structures::world_smp, { drop, 1u }, x + 0.5, y + 0.25, z + 0.5) == false)
			{
				inventory.add(client.player, drop, 1u);
				java_inventory.send_all(client);
			}
		}

		else if (smp && can_break(client, status))
		{
			change(structures::world_smp, x, y, z, structures::block_air);
		}

		else if (!smp && client.family == structures::family_1_8 && (status == v1_8_dig_start || status == v1_8_dig_finish))
		{
			restore_1_8(client, x, y, z);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::place(structures::client_s& client, std::int32_t x, std::int32_t y, std::int32_t z)
	{
		auto& stack{ client.player.inventory[client.player.held_slot] };
		auto kind{ stack.count ? items.get_block(stack.item) : kind_none };

		if (kind != kind_none && client.player.gamemode == gamemode_creative)
		{
			change(structures::world_smp, x, y, z, kind);
		}

		else if (kind != kind_none)
		{
			change(structures::world_smp, x, y, z, kind);
			stack.count = static_cast<std::uint8_t>(stack.count - 1u);
			stack.item = stack.count ? stack.item : 0u;
			java_inventory.send_slot(client, client.player.held_slot);
		}

		else
		{
			send_change(client, x, y, z, structures::block_air);
			java_inventory.send_slot(client, client.player.held_slot);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::change(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind)
	{
		auto radius{ static_cast<std::int32_t>(config.view_distance) };

		if (edits.set(target, x, y, z, kind))
		{
			java_chunk.invalidate(target, x >> 4, z >> 4);

			for (auto player : world.get_players())
			{
				auto* other{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };

				if (other && player->world == target && std::abs((x >> 4) - other->view_x) <= radius && std::abs((z >> 4) - other->view_z) <= radius)
				{
					send_change(*other, x, y, z, kind);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::send_change(structures::client_s& client, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind)
	{
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_block_update);
			functions::write_u64(packet, functions::encode_position(x, y, z));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_block_change);
			functions::write_u64(packet, functions::encode_position_1_8(x, y, z));
		}

		functions::write_varint(packet, static_cast<std::int32_t>(blocks.get_state(client.family, kind)));
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::restore_1_8(structures::client_s& client, std::int32_t x, std::int32_t y, std::int32_t z)
	{
		auto platform{ (x >> 4) == (spawn_block_x >> 4) && (z >> 4) == (spawn_block_z >> 4) && y == spawn_block_y && is_platform_column(x & 15, z & 15) };

		packet.clear();
		functions::write_varint(packet, v1_8_play_out_block_change);
		functions::write_u64(packet, functions::encode_position_1_8(x, y, z));
		functions::write_varint(packet, platform ? static_cast<std::int32_t>(v1_8_grass_block) : 0);
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	bool java_block_c::can_break(const structures::client_s& client, std::int32_t status)
	{
		return status == v1_8_dig_finish || (status == v1_8_dig_start && client.player.gamemode == gamemode_creative);
	}
}

//=====================================================================================
