
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_play_c java_play;

	void java_play_c::join(structures::client_s& client)
	{
		client.entity_id = world.allocate_entity_id();
		client.keep_alive_seen_ms = functions::get_time_ms();
		client.keep_alive_sent_ms = client.keep_alive_seen_ms;
		client.player.entity_id = client.entity_id;
		client.player.uuid_high = client.uuid_high;
		client.player.uuid_low = client.uuid_low;
		client.player.name = client.name;
		client.player.x = spawn_x;
		client.player.y = spawn_y;
		client.player.z = spawn_z;
		client.player.on_ground = true;
		client.player.edition = structures::edition_java;
		client.player.platform = bedrock_platform_java;
		client.player.link = &client;

		if (functions::is_modern(client.family))
		{
			join_modern(client);
		}

		else
		{
			join_1_8(client);
		}

		utils::log.write(color_play, "[play](join) '%s' on '%s' entity %d at %.1f %.1f %.1f", client.name.c_str(), functions::get_version_name(client.protocol), client.entity_id, spawn_x, spawn_y, spawn_z);
		world.add(client.player);
		chat.join(client.player);
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::join_modern(structures::client_s& client)
	{
		packet.clear();
		functions::write_varint(packet, client.layout.play_out_login);
		functions::write_i32(packet, client.entity_id);
		functions::write_bool(packet, false);
		functions::write_varint(packet, 1);
		functions::write_string(packet, modern_overworld);
		functions::write_varint(packet, static_cast<std::int32_t>(config.max_players));
		functions::write_varint(packet, static_cast<std::int32_t>(config.view_distance));
		functions::write_varint(packet, static_cast<std::int32_t>(config.view_distance));
		functions::write_bool(packet, false);
		functions::write_bool(packet, true);
		functions::write_bool(packet, false);
		functions::write_varint(packet, static_cast<std::int32_t>(java_registry.get(client.family).overworld));
		functions::write_string(packet, modern_overworld);
		functions::write_i64(packet, 0i64);
		functions::write_u8(packet, gamemode_survival);
		functions::write_u8(packet, client.layout.optional_previous_gamemode ? 0x00u : 0xFFu);
		functions::write_bool(packet, false);
		functions::write_bool(packet, true);
		functions::write_bool(packet, false);
		functions::write_varint(packet, 0);
		functions::write_varint(packet, modern_sea_level);
		functions::write_bool(packet, false);
		functions::write_bool(packet, false);
		network.send(client, packet);
		java_command.send_tree(client);

		java_chunk.send_view(client);
		teleport(client);

		packet.clear();
		functions::write_varint(packet, client.layout.play_out_game_event);
		functions::write_u8(packet, modern_game_event_wait_for_chunks);
		functions::write_f32(packet, 0.0f);
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::join_1_8(structures::client_s& client)
	{
		packet.clear();
		functions::write_varint(packet, v1_8_play_out_join_game);
		functions::write_i32(packet, client.entity_id);
		functions::write_u8(packet, gamemode_survival);
		functions::write_u8(packet, 0u);
		functions::write_u8(packet, 1u);
		functions::write_u8(packet, static_cast<std::uint8_t>(std::min(config.max_players, 255u)));
		functions::write_string(packet, v1_8_level_type);
		functions::write_bool(packet, false);
		network.send(client, packet);

		packet.clear();
		functions::write_varint(packet, v1_8_play_out_spawn_position);
		functions::write_u64(packet, functions::encode_position_1_8(spawn_block_x, spawn_block_y + 1, spawn_block_z));
		network.send(client, packet);

		network.send_shared(client, java_chunk.get_1_8());
		teleport(client);
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::handle(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		if (functions::is_modern(client.family))
		{
			handle_modern(client, id, in);
		}

		else
		{
			handle_1_8(client, id, in);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::handle_modern(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		if (id == client.layout.play_in_keep_alive && functions::read_i64(in) == client.keep_alive_id && in.good())
		{
			client.keep_alive_seen_ms = functions::get_time_ms();
		}

		else if (id == client.layout.play_in_move_position || id == client.layout.play_in_move_position_rotation)
		{
			auto x{ functions::read_f64(in) };
			auto y{ functions::read_f64(in) };
			auto z{ functions::read_f64(in) };
			auto yaw{ id == client.layout.play_in_move_position_rotation ? std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) : client.player.yaw };
			auto pitch{ id == client.layout.play_in_move_position_rotation ? std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) : client.player.pitch };
			auto flags{ functions::read_u8(in) };

			if (in.good())
			{
				apply_movement(client, x, y, z, yaw, pitch, (flags & 1u) != 0u);
			}
		}

		else if (id == client.layout.play_in_move_rotation)
		{
			auto yaw{ std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) };
			auto pitch{ std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) };
			auto flags{ functions::read_u8(in) };

			if (in.good())
			{
				apply_movement(client, client.player.x, client.player.y, client.player.z, yaw, pitch, (flags & 1u) != 0u);
			}
		}

		else if (id == client.layout.play_in_chat)
		{
			java_chat.handle(client, in);
		}

		else if (id == client.layout.play_in_chat_command)
		{
			java_chat.handle_command(client, in);
		}

		else if (id == client.layout.play_in_player_action || id == client.layout.play_in_use_item_on || id == client.layout.play_in_use_item)
		{
			java_block.handle_modern(client, id, in);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::handle_1_8(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		if (id == v1_8_play_in_keep_alive && functions::read_varint(in) == client.keep_alive_id && in.good())
		{
			client.keep_alive_seen_ms = functions::get_time_ms();
		}

		else if (id == v1_8_play_in_position || id == v1_8_play_in_position_look)
		{
			auto x{ functions::read_f64(in) };
			auto y{ functions::read_f64(in) };
			auto z{ functions::read_f64(in) };
			auto yaw{ id == v1_8_play_in_position_look ? std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) : client.player.yaw };
			auto pitch{ id == v1_8_play_in_position_look ? std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) : client.player.pitch };
			auto ground{ functions::read_u8(in) };

			if (in.good())
			{
				apply_movement(client, x, y, z, yaw, pitch, ground != 0u);
			}
		}

		else if (id == v1_8_play_in_look)
		{
			auto yaw{ std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) };
			auto pitch{ std::bit_cast<std::float_t>(static_cast<std::uint32_t>(functions::read_u32(in))) };
			auto ground{ functions::read_u8(in) };

			if (in.good())
			{
				apply_movement(client, client.player.x, client.player.y, client.player.z, yaw, pitch, ground != 0u);
			}
		}

		else if (id == v1_8_play_in_chat)
		{
			java_chat.handle(client, in);
		}

		else if (id == v1_8_play_in_dig)
		{
			java_block.handle_dig_1_8(client, in);
		}

		else if (id == v1_8_play_in_place)
		{
			java_block.handle_place_1_8(client, in);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::apply_movement(structures::client_s& client, std::double_t x, std::double_t y, std::double_t z, std::float_t yaw, std::float_t pitch, bool on_ground)
	{
		if (y < rescue_y)
		{
			teleport(client);
			client.player.x = spawn_x;
			client.player.y = spawn_y;
			client.player.z = spawn_z;
			world.moved(client.player);
			utils::log.write(color_play, "[play](rescue) '%s' fell into the void", client.name.c_str());
		}

		else if (std::isfinite(x) && std::isfinite(y) && std::isfinite(z) && std::isfinite(yaw) && std::isfinite(pitch))
		{
			client.player.x = x;
			client.player.y = y;
			client.player.z = z;
			client.player.yaw = yaw;
			client.player.pitch = pitch;
			client.player.on_ground = on_ground;
			world.moved(client.player);
		}

		if (functions::is_modern(client.family))
		{
			java_chunk.update_view(client);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::teleport(structures::client_s& client)
	{
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_position);
			functions::write_varint(packet, client.entity_id);
			functions::write_f64(packet, spawn_x);
			functions::write_f64(packet, spawn_y);
			functions::write_f64(packet, spawn_z);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f32(packet, 0.0f);
			functions::write_f32(packet, 0.0f);
			functions::write_i32(packet, 0);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_position);
			functions::write_f64(packet, spawn_x);
			functions::write_f64(packet, spawn_y);
			functions::write_f64(packet, spawn_z);
			functions::write_f32(packet, 0.0f);
			functions::write_f32(packet, 0.0f);
			functions::write_u8(packet, 0u);
		}

		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::send_keep_alive(structures::client_s& client, std::uint64_t now)
	{
		client.keep_alive_id = static_cast<std::int64_t>(now & 0x7FFFFFFFui64);
		client.keep_alive_sent_ms = now;

		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_keep_alive);
			functions::write_i64(packet, client.keep_alive_id);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_keep_alive);
			functions::write_varint(packet, static_cast<std::int32_t>(client.keep_alive_id));
		}

		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::tick(structures::client_s& client, std::uint64_t now)
	{
		if (now - client.keep_alive_seen_ms > keep_alive_timeout_ms)
		{
			java_protocol.kick(client, "timed out");
		}

		else if (now - client.keep_alive_sent_ms >= keep_alive_interval_ms)
		{
			send_keep_alive(client, now);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_play_c::leave(structures::client_s& client)
	{
		world.remove(client.player);
		utils::log.write(color_play, "[play](leave) '%s' on '%s'", client.name.c_str(), functions::get_version_name(client.protocol));
	}
}

//=====================================================================================
