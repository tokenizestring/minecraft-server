
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

			if (in.good() && smp && can_break(client, status))
			{
				change(structures::world_smp, position[0], position[1], position[2], structures::block_air);
			}
		}

		else if (id == client.layout.play_in_use_item_on)
		{
			auto hand{ functions::read_varint(in) };
			auto position{ functions::decode_position(functions::read_u64(in)) };
			auto face{ static_cast<std::uint32_t>(functions::read_varint(in)) };
			auto kind{ blocks.find_item(client.family, client.hotbar[client.held_slot]) };

			if (in.good() && smp && hand == 0 && face < std::size(face_offsets) && kind != kind_none)
			{
				change(structures::world_smp, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2], kind);
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
	void java_block_c::handle_held(structures::client_s& client, structures::reader_s& in)
	{
		auto slot{ functions::read_u16(in) };

		if (in.good() && slot < hotbar_size)
		{
			client.held_slot = slot;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::handle_creative(structures::client_s& client, structures::reader_s& in)
	{
		auto slot{ static_cast<std::int16_t>(functions::read_u16(in)) - hotbar_first_slot };
		auto count{ functions::read_varint(in) };
		auto item{ count > 0 ? functions::read_varint(in) : 0 };

		if (in.good() && slot >= 0 && slot < static_cast<std::int32_t>(hotbar_size))
		{
			client.hotbar[static_cast<std::size_t>(slot)] = item;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::handle_dig_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto status{ functions::read_u8(in) };
		auto position{ functions::decode_position_1_8(functions::read_u64(in)) };
		auto smp{ client.player.world == structures::world_smp };

		if (in.good() && smp && can_break(client, status))
		{
			change(structures::world_smp, position[0], position[1], position[2], structures::block_air);
		}

		else if (in.good() && !smp && (status == v1_8_dig_start || status == v1_8_dig_finish))
		{
			restore_1_8(client, position[0], position[1], position[2]);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_block_c::handle_place_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto position{ functions::decode_position_1_8(functions::read_u64(in)) };
		auto face{ functions::read_u8(in) };
		auto item{ static_cast<std::int16_t>(functions::read_u16(in)) };
		auto count{ item >= 0 ? functions::read_u8(in) : 0u };
		auto damage{ item >= 0 ? functions::read_u16(in) : 0u };
		auto kind{ count ? blocks.find_item_1_8(item, damage) : kind_none };
		auto smp{ client.player.world == structures::world_smp };
		auto valid{ in.good() && face < std::size(face_offsets) };

		if (valid && smp && kind != kind_none)
		{
			change(structures::world_smp, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2], kind);
		}

		else if (valid && smp)
		{
			send_change(client, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2], structures::block_air);
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
