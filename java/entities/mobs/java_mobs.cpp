
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_mobs_c java_mobs;

	void java_mobs_c::spawn(structures::client_s& viewer, const structures::mob_s& mob)
	{
		auto yaw{ functions::get_angle_byte(mob.yaw) };
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_add_entity);
			functions::write_varint(packet, mob.entity_id);
			functions::write_u64(packet, mob.uuid_high);
			functions::write_u64(packet, mob.uuid_low);
			functions::write_varint(packet, viewer.layout.mob_types[mob.kind]);
			functions::write_f64(packet, mob.x);
			functions::write_f64(packet, mob.y);
			functions::write_f64(packet, mob.z);
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, yaw);
			functions::write_u8(packet, yaw);
			functions::write_varint(packet, 0);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_spawn_mob);
			functions::write_varint(packet, mob.entity_id);
			functions::write_u8(packet, static_cast<std::uint8_t>(mob_types_1_8[mob.kind]));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(mob.x * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(mob.y * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(mob.z * v1_8_fixed_scale)));
			functions::write_u8(packet, yaw);
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, yaw);
			functions::write_u16(packet, 0u);
			functions::write_u16(packet, 0u);
			functions::write_u16(packet, 0u);
			functions::write_u8(packet, v1_8_metadata_health);
			functions::write_f32(packet, mob.health);
			functions::write_u8(packet, v1_8_metadata_end);
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_mobs_c::move(structures::client_s& viewer, const structures::mob_s& mob, bool moved, bool turned)
	{
		auto modern{ functions::is_modern(viewer.family) };
		auto scale{ modern ? java_move_scale : v1_8_fixed_scale };
		auto limit{ static_cast<std::int64_t>((modern ? java_move_limit : v1_8_move_limit) * scale) };
		auto step_x{ get_step(mob.sent_x, mob.x, scale) };
		auto step_y{ get_step(mob.sent_y, mob.y, scale) };
		auto step_z{ get_step(mob.sent_z, mob.z, scale) };
		auto relative{ std::abs(step_x) < limit && std::abs(step_y) < limit && std::abs(step_z) < limit };
		auto yaw{ functions::get_angle_byte(mob.yaw) };

		if (moved && relative)
		{
			packet.clear();
			functions::write_varint(packet, modern ? viewer.layout.play_out_move_entity_pos : v1_8_play_out_entity_move);
			functions::write_varint(packet, mob.entity_id);

			if (modern && viewer.layout.packed_move)
			{
				functions::write_varint(packet, 1);
			}

			if (modern)
			{
				functions::write_u16(packet, static_cast<std::uint16_t>(static_cast<std::int16_t>(step_x)));
				functions::write_u16(packet, static_cast<std::uint16_t>(static_cast<std::int16_t>(step_y)));
				functions::write_u16(packet, static_cast<std::uint16_t>(static_cast<std::int16_t>(step_z)));
			}

			else
			{
				functions::write_u8(packet, static_cast<std::uint8_t>(static_cast<std::int8_t>(step_x)));
				functions::write_u8(packet, static_cast<std::uint8_t>(static_cast<std::int8_t>(step_y)));
				functions::write_u8(packet, static_cast<std::uint8_t>(static_cast<std::int8_t>(step_z)));
			}

			if (modern == false || viewer.layout.packed_move == false)
			{
				functions::write_bool(packet, true);
			}

			network.send(viewer, packet);
		}

		else if (moved)
		{
			write_teleport(viewer, mob);
			network.send(viewer, packet);
		}

		if (turned)
		{
			packet.clear();
			functions::write_varint(packet, modern ? viewer.layout.play_out_move_entity_rot : v1_8_play_out_entity_look);
			functions::write_varint(packet, mob.entity_id);

			if (modern && viewer.layout.packed_move)
			{
				functions::write_bool(packet, true);
			}

			functions::write_u8(packet, yaw);
			functions::write_u8(packet, 0u);

			if (modern == false || viewer.layout.packed_move == false)
			{
				functions::write_bool(packet, true);
			}

			network.send(viewer, packet);

			packet.clear();
			functions::write_varint(packet, modern ? viewer.layout.play_out_rotate_head : v1_8_play_out_head_look);
			functions::write_varint(packet, mob.entity_id);
			functions::write_u8(packet, yaw);
			network.send(viewer, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_mobs_c::hurt(structures::client_s& viewer, const structures::mob_s& mob)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_hurt_animation);
			functions::write_varint(packet, mob.entity_id);
			functions::write_f32(packet, 0.0f);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_entity_status);
			functions::write_i32(packet, mob.entity_id);
			functions::write_u8(packet, entity_event_hurt);
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_mobs_c::die(structures::client_s& viewer, const structures::mob_s& mob)
	{
		auto modern{ functions::is_modern(viewer.family) };

		packet.clear();
		functions::write_varint(packet, modern ? viewer.layout.play_out_entity_data : v1_8_play_out_entity_metadata);
		functions::write_varint(packet, mob.entity_id);

		if (modern)
		{
			functions::write_u8(packet, java_metadata_health_index);
			functions::write_varint(packet, java_metadata_float_type);
			functions::write_f32(packet, 0.0f);
			functions::write_u8(packet, java_metadata_end);
		}

		else
		{
			functions::write_u8(packet, v1_8_metadata_health);
			functions::write_f32(packet, 0.0f);
			functions::write_u8(packet, v1_8_metadata_end);
		}

		network.send(viewer, packet);

		packet.clear();
		functions::write_varint(packet, modern ? viewer.layout.play_out_entity_event : v1_8_play_out_entity_status);
		functions::write_i32(packet, mob.entity_id);
		functions::write_u8(packet, entity_event_death);
		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_mobs_c::sound(structures::client_s& viewer, const structures::mob_s& mob, std::uint32_t which)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_sound_entity);
			functions::write_varint(packet, viewer.layout.mob_sounds[mob.kind][which] + 1);
			functions::write_varint(packet, sound_source_neutral);
			functions::write_varint(packet, mob.entity_id);
			functions::write_f32(packet, 1.0f);
			functions::write_f32(packet, 1.0f);
			functions::write_i64(packet, static_cast<std::int64_t>(mob.uuid_low));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_named_sound);
			functions::write_string(packet, v1_8_mob_sounds[mob.kind][which]);
			functions::write_i32(packet, static_cast<std::int32_t>(mob.x * v1_8_sound_scale));
			functions::write_i32(packet, static_cast<std::int32_t>(mob.y * v1_8_sound_scale));
			functions::write_i32(packet, static_cast<std::int32_t>(mob.z * v1_8_sound_scale));
			functions::write_f32(packet, 1.0f);
			functions::write_u8(packet, static_cast<std::uint8_t>(v1_8_sound_pitch));
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_mobs_c::remove(structures::client_s& viewer, std::int32_t entity_id)
	{
		packet.clear();
		functions::write_varint(packet, functions::is_modern(viewer.family) ? viewer.layout.play_out_remove_entities : v1_8_play_out_destroy_entities);
		functions::write_varint(packet, 1);
		functions::write_varint(packet, entity_id);
		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_mobs_c::write_teleport(const structures::client_s& viewer, const structures::mob_s& mob)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_teleport_entity);
			functions::write_varint(packet, mob.entity_id);
			functions::write_f64(packet, mob.x);
			functions::write_f64(packet, mob.y);
			functions::write_f64(packet, mob.z);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f32(packet, mob.yaw);
			functions::write_f32(packet, 0.0f);
			functions::write_i32(packet, 0);
			functions::write_bool(packet, true);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_entity_teleport);
			functions::write_varint(packet, mob.entity_id);
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(mob.x * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(mob.y * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(mob.z * v1_8_fixed_scale)));
			functions::write_u8(packet, functions::get_angle_byte(mob.yaw));
			functions::write_u8(packet, 0u);
			functions::write_bool(packet, true);
		}
	}
	/*
	//=====================================================================================
	*/
	std::int64_t java_mobs_c::get_step(std::double_t from, std::double_t to, std::double_t scale)
	{
		return static_cast<std::int64_t>(std::floor(to * scale)) - static_cast<std::int64_t>(std::floor(from * scale));
	}
}

//=====================================================================================
