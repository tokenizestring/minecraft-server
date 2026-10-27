
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_player_c java_player;

	void java_player_c::spawn(structures::client_s& viewer, const structures::player_s& subject, bool entity)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_player_info_update);
			functions::write_u8(packet, java_info_actions);
			functions::write_varint(packet, 1);
			functions::write_u64(packet, subject.uuid_high);
			functions::write_u64(packet, subject.uuid_low);
			functions::write_string(packet, functions::get_java_name(subject.name));
			functions::write_varint(packet, 0);
			functions::write_bool(packet, false);
			functions::write_varint(packet, 0);
			functions::write_bool(packet, true);
			functions::write_varint(packet, 0);
			functions::write_bool(packet, false);
			functions::write_varint(packet, 0);
			functions::write_bool(packet, false);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_player_list);
			functions::write_varint(packet, v1_8_player_list_add);
			functions::write_varint(packet, 1);
			functions::write_u64(packet, subject.uuid_high);
			functions::write_u64(packet, subject.uuid_low);
			functions::write_string(packet, functions::get_java_name(subject.name));
			functions::write_varint(packet, 0);
			functions::write_varint(packet, 0);
			functions::write_varint(packet, 0);
			functions::write_bool(packet, false);
		}

		network.send(viewer, packet);

		if (entity)
		{
			packet.clear();

			if (functions::is_modern(viewer.family))
			{
				functions::write_varint(packet, viewer.layout.play_out_add_entity);
				functions::write_varint(packet, subject.entity_id);
				functions::write_u64(packet, subject.uuid_high);
				functions::write_u64(packet, subject.uuid_low);
				functions::write_varint(packet, viewer.layout.player_entity_type);
				functions::write_f64(packet, subject.x);
				functions::write_f64(packet, subject.y);
				functions::write_f64(packet, subject.z);
				functions::write_u8(packet, 0u);
				functions::write_u8(packet, functions::get_angle_byte(subject.pitch));
				functions::write_u8(packet, functions::get_angle_byte(subject.yaw));
				functions::write_u8(packet, functions::get_angle_byte(subject.yaw));
				functions::write_varint(packet, 0);
			}

			else
			{
				functions::write_varint(packet, v1_8_play_out_spawn_player);
				functions::write_varint(packet, subject.entity_id);
				functions::write_u64(packet, subject.uuid_high);
				functions::write_u64(packet, subject.uuid_low);
				functions::write_i32(packet, static_cast<std::int32_t>(std::floor(subject.x * v1_8_fixed_scale)));
				functions::write_i32(packet, static_cast<std::int32_t>(std::floor(subject.y * v1_8_fixed_scale)));
				functions::write_i32(packet, static_cast<std::int32_t>(std::floor(subject.z * v1_8_fixed_scale)));
				functions::write_u8(packet, functions::get_angle_byte(subject.yaw));
				functions::write_u8(packet, functions::get_angle_byte(subject.pitch));
				functions::write_u16(packet, 0u);
				functions::write_u8(packet, v1_8_metadata_health);
				functions::write_f32(packet, v1_8_health);
				functions::write_u8(packet, v1_8_metadata_end);
			}

			network.send(viewer, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_player_c::move(structures::client_s& viewer, const structures::player_s& subject)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_teleport_entity);
			functions::write_varint(packet, subject.entity_id);
			functions::write_f64(packet, subject.x);
			functions::write_f64(packet, subject.y);
			functions::write_f64(packet, subject.z);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f32(packet, subject.yaw);
			functions::write_f32(packet, subject.pitch);
			functions::write_i32(packet, 0);
			functions::write_bool(packet, subject.on_ground);
			network.send(viewer, packet);

			packet.clear();
			functions::write_varint(packet, viewer.layout.play_out_rotate_head);
			functions::write_varint(packet, subject.entity_id);
			functions::write_u8(packet, functions::get_angle_byte(subject.yaw));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_entity_teleport);
			functions::write_varint(packet, subject.entity_id);
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(subject.x * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(subject.y * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(subject.z * v1_8_fixed_scale)));
			functions::write_u8(packet, functions::get_angle_byte(subject.yaw));
			functions::write_u8(packet, functions::get_angle_byte(subject.pitch));
			functions::write_bool(packet, subject.on_ground);
			network.send(viewer, packet);

			packet.clear();
			functions::write_varint(packet, v1_8_play_out_head_look);
			functions::write_varint(packet, subject.entity_id);
			functions::write_u8(packet, functions::get_angle_byte(subject.yaw));
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_player_c::despawn(structures::client_s& viewer, const structures::player_s& subject)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_remove_entities);
			functions::write_varint(packet, 1);
			functions::write_varint(packet, subject.entity_id);
			network.send(viewer, packet);

			packet.clear();
			functions::write_varint(packet, viewer.layout.play_out_player_info_remove);
			functions::write_varint(packet, 1);
			functions::write_u64(packet, subject.uuid_high);
			functions::write_u64(packet, subject.uuid_low);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_destroy_entities);
			functions::write_varint(packet, 1);
			functions::write_varint(packet, subject.entity_id);
			network.send(viewer, packet);

			packet.clear();
			functions::write_varint(packet, v1_8_play_out_player_list);
			functions::write_varint(packet, v1_8_player_list_remove);
			functions::write_varint(packet, 1);
			functions::write_u64(packet, subject.uuid_high);
			functions::write_u64(packet, subject.uuid_low);
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_player_c::set_gamemode(structures::client_s& client, std::uint8_t mode)
	{
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_game_event);
			functions::write_u8(packet, modern_game_event_change_mode);
			functions::write_f32(packet, static_cast<std::float_t>(mode));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_game_state);
			functions::write_u8(packet, modern_game_event_change_mode);
			functions::write_f32(packet, static_cast<std::float_t>(mode));
		}

		network.send(client, packet);
	}
}

//=====================================================================================
