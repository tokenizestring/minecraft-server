
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_play_c bedrock_play;

	void bedrock_play_c::start_game(structures::session_s& session)
	{
		body.clear();
		functions::write_bytes(body, bedrock_empty_jigsaws, sizeof(bedrock_empty_jigsaws));
		bedrock_packet.send(session, bedrock_jigsaw_structure_data, body);

		body.clear();
		functions::write_varint(body, 0);
		functions::write_varint(body, 0);
		functions::write_u16_le(body, 0u);
		if (auto file{ std::fopen(functions::get_local_path("../../tools/captures/capture_bedrock_bds_nogen/009_337.bin").c_str(), "rb") }; file && bedrock_debug_reference)
		{
			body.resize(1048576u);
			body.resize(std::fread(body.data(), 1u, body.size(), file));
			std::fclose(file);
		}

		bedrock_packet.send(session, bedrock_voxel_shapes, body);

		body.clear();
		write_start_game(body, session);
		bedrock_packet.send(session, bedrock_start_game, body);

		body.clear();
		functions::write_varint(body, 0);
		if (auto file{ std::fopen(functions::get_local_path("../../tools/captures/capture_bedrock_bds_nogen/024_162.bin").c_str(), "rb") }; file && bedrock_debug_reference_data)
		{
			body.resize(1048576u);
			body.resize(std::fread(body.data(), 1u, body.size(), file));
			std::fclose(file);
		}

		bedrock_packet.send(session, bedrock_item_registry, body);

		body.clear();
		functions::write_varint(body, 0);
		functions::write_varint(body, 0);
		if (auto file{ std::fopen(functions::get_local_path("../../tools/captures/capture_bedrock_bds_nogen/034_122.bin").c_str(), "rb") }; file && bedrock_debug_reference_data)
		{
			body.resize(1048576u);
			body.resize(std::fread(body.data(), 1u, body.size(), file));
			std::fclose(file);
		}

		bedrock_packet.send(session, bedrock_biome_definition_list, body);

		body.clear();
		functions::write_zigzag32(body, bedrock_view_radius);
		bedrock_packet.send(session, bedrock_chunk_radius_updated, body);

		body.clear();
		functions::write_zigzag32(body, spawn_block_x);
		functions::write_zigzag32(body, spawn_block_y);
		functions::write_zigzag32(body, spawn_block_z);
		functions::write_varint(body, bedrock_view_radius << 4);
		functions::write_u32_le(body, 0u);
		bedrock_packet.send(session, bedrock_chunk_publisher_update, body);

		body.clear();
		functions::write_f32_le(body, static_cast<std::float_t>(spawn_x));
		functions::write_f32_le(body, static_cast<std::float_t>(spawn_y) + bedrock_eye_height);
		functions::write_f32_le(body, static_cast<std::float_t>(spawn_z));
		functions::write_u8(body, bedrock_respawn_ready);
		functions::write_varint64(body, 0ui64);
		bedrock_packet.send(session, bedrock_respawn, body);

		if (bedrock_debug_send_world)
		{
			raknet.send(session, bedrock_chunk.get_spawn_area(), raknet_reliability_reliable_ordered);
		}

		body.clear();
		functions::write_bool(body, false);
		functions::write_bool(body, false);
		functions::write_bool(body, true);
		functions::write_bool(body, true);
		functions::write_bool(body, true);
		bedrock_packet.send(session, bedrock_update_adventure_settings, body);

		bedrock_packet.send_play_status(session, bedrock_status_player_spawn);
		bedrock_player.send_actor_data(session);

		utils::log.write(color_play, "[play](spawn) '%s' start game sent, %.1f kb spawn_area", session.name.c_str(), static_cast<std::double_t>(bedrock_chunk.get_spawn_area().size()) / 1024.0);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_play_c::write_start_game(std::vector<std::uint8_t>& out, const structures::session_s& session)
	{
		functions::write_zigzag64(out, session.entity_id);
		functions::write_varint64(out, static_cast<std::uint64_t>(session.entity_id));
		functions::write_zigzag32(out, gamemode_survival);
		functions::write_f32_le(out, static_cast<std::float_t>(spawn_x));
		functions::write_f32_le(out, static_cast<std::float_t>(spawn_y) + bedrock_eye_height);
		functions::write_f32_le(out, static_cast<std::float_t>(spawn_z));
		functions::write_f32_le(out, 0.0f);
		functions::write_f32_le(out, spawn_yaw);

		functions::write_u64_le(out, 0ui64);
		functions::write_u16_le(out, 0u);
		functions::write_string(out, "");
		functions::write_zigzag32(out, 0);
		functions::write_zigzag32(out, bedrock_generator_flat);
		functions::write_zigzag32(out, gamemode_survival);
		functions::write_bool(out, false);
		functions::write_zigzag32(out, bedrock_difficulty_easy);
		functions::write_zigzag32(out, spawn_block_x);
		functions::write_zigzag32(out, spawn_block_y + 1);
		functions::write_zigzag32(out, spawn_block_z);
		functions::write_bool(out, true);
		functions::write_zigzag32(out, 0);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_zigzag32(out, 0);
		functions::write_varint(out, 0);
		functions::write_bool(out, false);
		functions::write_string(out, "");
		functions::write_f32_le(out, 0.0f);
		functions::write_f32_le(out, 0.0f);
		functions::write_bool(out, false);
		functions::write_bool(out, true);
		functions::write_bool(out, true);
		functions::write_zigzag32(out, 0);
		functions::write_zigzag32(out, 0);
		functions::write_bool(out, true);
		functions::write_bool(out, false);
		functions::write_varint(out, 0);
		functions::write_u32_le(out, 0u);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_zigzag32(out, bedrock_permission_member);
		functions::write_u32_le(out, static_cast<std::uint32_t>(bedrock_chunk_tick_range));
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_string(out, bedrock_any_version);
		functions::write_u32_le(out, 0u);
		functions::write_u32_le(out, 0u);
		functions::write_bool(out, false);
		functions::write_string(out, "");
		functions::write_string(out, "");
		functions::write_bool(out, false);
		functions::write_u8(out, 0u);
		functions::write_bool(out, false);
		functions::write_zigzag32(out, 0);
		functions::write_bool(out, false);

		functions::write_string(out, bedrock_level_name);
		functions::write_string(out, bedrock_level_name);
		functions::write_string(out, bedrock_zero_uuid);
		functions::write_bool(out, false);
		functions::write_zigzag32(out, 0);
		functions::write_bool(out, false);
		functions::write_u64_le(out, 0ui64);
		functions::write_zigzag32(out, 0);
		functions::write_varint(out, 0);
		functions::write_string(out, bedrock_correlation_id);
		functions::write_bool(out, true);
		functions::write_string(out, bedrock_server_version);
		functions::write_u8(out, 0x0Au);
		functions::write_u8(out, 0x00u);
		functions::write_u8(out, 0x00u);
		functions::write_u64_le(out, 0ui64);
		functions::write_u64_le(out, 0ui64);
		functions::write_u64_le(out, 0ui64);
		functions::write_bool(out, false);
		functions::write_bool(out, true);
		functions::write_bool(out, false);
		functions::write_bytes(out, bedrock_start_game_tail, sizeof(bedrock_start_game_tail));
	}
	/*
	//=====================================================================================
	*/
	void bedrock_play_c::handle_chunk_radius(structures::session_s& session, structures::reader_s& in)
	{
		functions::read_varint(in);

		body.clear();
		functions::write_zigzag32(body, bedrock_view_radius);
		bedrock_packet.send(session, bedrock_chunk_radius_updated, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_play_c::handle_auth_input(structures::session_s& session, structures::reader_s& in)
	{
		std::array<std::float_t, 5u> values{};

		if (functions::read_raw(in, values.data(), sizeof(values)) && session.playing && values[3] - bedrock_eye_height < rescue_y)
		{
			teleport(session);
			session.player.x = spawn_x;
			session.player.y = spawn_y;
			session.player.z = spawn_z;
			world.moved(session.player);
			utils::log.write(color_play, "[play](rescue) '%s' fell into the void", session.name.c_str());
		}

		else if (in.good() && session.playing && (values[0] != session.player.pitch || values[1] != session.player.yaw || values[2] != session.player.x || values[3] - bedrock_eye_height != session.player.y || values[4] != session.player.z))
		{
			session.player.pitch = values[0];
			session.player.yaw = values[1];
			session.player.x = values[2];
			session.player.y = values[3] - bedrock_eye_height;
			session.player.z = values[4];
			session.player.on_ground = true;
			world.moved(session.player);
		}

		bedrock_debug.inject(session);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_play_c::join(structures::session_s& session)
	{
		session.playing = true;
		session.player.entity_id = static_cast<std::int32_t>(session.entity_id);
		functions::make_offline_uuid(session.name, session.player.uuid_high, session.player.uuid_low);
		session.player.name = session.name;
		session.player.x = spawn_x;
		session.player.y = spawn_y;
		session.player.z = spawn_z;
		session.player.yaw = spawn_yaw;
		session.player.world = structures::world_spawn;
		session.player.on_ground = true;
		session.player.edition = structures::edition_bedrock;
		session.player.link = &session;
		session.inject_ms = functions::get_time_ms() + bedrock_inject_interval_ms;
		profile.load(session.player);

		utils::log.write(color_play, "[play](join) '%s' on 'bedrock %s' entity %d at %.1f %.1f %.1f", session.name.c_str(), bedrock_version, session.player.entity_id, spawn_x, spawn_y, spawn_z);
		world.add(session.player);
		chat.join(session.player);
		friends.joined(session.player);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_play_c::leave(structures::session_s& session)
	{
		if (session.playing)
		{
			profile.save(session.player);
		}

		world.remove(session.player);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_play_c::teleport(structures::session_s& session)
	{
		body.clear();
		functions::write_varint64(body, static_cast<std::uint64_t>(session.entity_id));
		functions::write_f32_le(body, static_cast<std::float_t>(spawn_x));
		functions::write_f32_le(body, static_cast<std::float_t>(spawn_y) + bedrock_eye_height);
		functions::write_f32_le(body, static_cast<std::float_t>(spawn_z));
		functions::write_f32_le(body, 0.0f);
		functions::write_f32_le(body, spawn_yaw);
		functions::write_f32_le(body, spawn_yaw);
		functions::write_u8(body, bedrock_move_teleport);
		functions::write_bool(body, false);
		functions::write_varint64(body, 0ui64);
		functions::write_bool(body, true);
		functions::write_u32_le(body, bedrock_teleport_cause_unknown);
		functions::write_u32_le(body, 0u);
		functions::write_varint64(body, 0ui64);
		bedrock_packet.send(session, bedrock_move_player, body);
	}
}

//=====================================================================================
