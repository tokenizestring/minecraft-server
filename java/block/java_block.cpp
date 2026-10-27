
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_block_c java_block;

	void java_block_c::handle_modern(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		auto sequence{ functions::read_trailing_varint(in) };

		if (id == client.layout.play_in_use_item)
		{
			functions::read_varint(in);
			sequence = functions::read_varint(in);
		}

		else if (id == client.layout.play_in_player_action)
		{
			functions::read_varint(in);
			functions::read_u64(in);
			functions::read_u8(in);
			sequence = functions::read_varint(in);
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
		auto status{ functions::read_u8(in) };
		auto position{ functions::decode_position_1_8(functions::read_u64(in)) };

		if (in.good() && (status == v1_8_dig_start || status == v1_8_dig_finish))
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

		if (in.good() && face < std::size(face_offsets))
		{
			restore_1_8(client, position[0], position[1], position[2]);
			restore_1_8(client, position[0] + face_offsets[face][0], position[1] + face_offsets[face][1], position[2] + face_offsets[face][2]);
		}
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
}

//=====================================================================================
