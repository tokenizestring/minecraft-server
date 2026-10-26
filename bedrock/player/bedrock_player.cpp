
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_player_c bedrock_player;

	void bedrock_player_c::build()
	{
		auto skin{ std::make_shared<structures::skin_s>() };
		skin->id = bedrock_skin_id;
		skin->patch = bedrock_skin_patch;
		skin->geometry = bedrock_skin_geometry;
		skin->engine = bedrock_skin_engine;
		skin->width = bedrock_skin_size;
		skin->height = bedrock_skin_size;
		skin->arm = bedrock_arm_wide;

		for (auto y{ 0u }; y < bedrock_skin_size; ++y)
		{
			for (auto x{ 0u }; x < bedrock_skin_size; ++x)
			{
				auto color{ 0u };

				if (y < 16u && x < 32u)
				{
					color = bedrock_skin_colors[0];
				}

				else if (y >= 16u && y < 32u)
				{
					color = bedrock_skin_colors[1];
				}

				else if (y >= 48u && x >= 16u && x < 32u)
				{
					color = bedrock_skin_colors[2];
				}

				else if (y >= 48u && x >= 32u && x < 48u)
				{
					color = bedrock_skin_colors[0];
				}

				functions::write_u32_le(skin->image, color);
			}
		}

		default_skin = std::move(skin);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_player_c::spawn(structures::session_s& viewer, const structures::player_s& subject, bool entity)
	{
		body.clear();
		functions::write_varint(body, 1);
		functions::write_varint(body, static_cast<std::int32_t>(bedrock_list_add));
		functions::write_u8(body, bedrock_list_action_add);
		functions::write_u64_le(body, subject.uuid_high);
		functions::write_u64_le(body, subject.uuid_low);
		functions::write_zigzag64(body, subject.entity_id);
		functions::write_string(body, subject.name);
		functions::write_string(body, "");
		functions::write_string(body, "");
		functions::write_u32_le(body, static_cast<std::uint32_t>(subject.platform));
		write_skin(body, subject.skin ? *subject.skin : *default_skin);
		functions::write_u8(body, 0u);
		functions::write_u8(body, 0u);
		functions::write_u8(body, 0u);
		functions::write_u32_le(body, 0u);
		bedrock_packet.send(viewer, bedrock_player_list, body);

		if (entity)
		{
			body.clear();
			functions::write_u64_le(body, subject.uuid_high);
			functions::write_u64_le(body, subject.uuid_low);
			functions::write_string(body, subject.name);
			functions::write_varint64(body, static_cast<std::uint64_t>(subject.entity_id));
			functions::write_string(body, "");
			functions::write_f32_le(body, static_cast<std::float_t>(subject.x));
			functions::write_f32_le(body, static_cast<std::float_t>(subject.y));
			functions::write_f32_le(body, static_cast<std::float_t>(subject.z));
			functions::write_f32_le(body, 0.0f);
			functions::write_f32_le(body, 0.0f);
			functions::write_f32_le(body, 0.0f);
			functions::write_f32_le(body, subject.pitch);
			functions::write_f32_le(body, subject.yaw);
			functions::write_f32_le(body, subject.yaw);
			functions::write_bytes(body, bedrock_empty_item, sizeof(bedrock_empty_item));
			functions::write_zigzag32(body, bedrock_gametype_default);
			write_actor_data(body, subject.name, false);
			functions::write_varint(body, 0);
			functions::write_varint(body, 0);
			functions::write_u64_le(body, static_cast<std::uint64_t>(subject.entity_id));
			functions::write_u8(body, bedrock_permission_member_byte);
			functions::write_u8(body, 0u);
			functions::write_varint(body, static_cast<std::int32_t>(std::size(bedrock_ability_layers)));

			for (auto index{ 0u }; index < std::size(bedrock_ability_layers); ++index)
			{
				functions::write_u16_le(body, bedrock_ability_layers[index]);
				functions::write_u32_le(body, bedrock_ability_sets[index]);
				functions::write_u32_le(body, bedrock_ability_values[index]);
				functions::write_f32_le(body, bedrock_ability_speeds[index * 3u]);
				functions::write_f32_le(body, bedrock_ability_speeds[index * 3u + 1u]);
				functions::write_f32_le(body, bedrock_ability_speeds[index * 3u + 2u]);
			}

			functions::write_varint(body, 0);
			functions::write_string(body, "");
			functions::write_u32_le(body, static_cast<std::uint32_t>(subject.platform));
			bedrock_packet.send(viewer, bedrock_add_player, body);
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_player_c::move(structures::session_s& viewer, const structures::player_s& subject)
	{
		body.clear();
		functions::write_varint64(body, static_cast<std::uint64_t>(subject.entity_id));
		functions::write_f32_le(body, static_cast<std::float_t>(subject.x));
		functions::write_f32_le(body, static_cast<std::float_t>(subject.y) + bedrock_eye_height);
		functions::write_f32_le(body, static_cast<std::float_t>(subject.z));
		functions::write_f32_le(body, subject.pitch);
		functions::write_f32_le(body, subject.yaw);
		functions::write_f32_le(body, subject.yaw);
		functions::write_u8(body, bedrock_move_normal);
		functions::write_bool(body, subject.on_ground);
		functions::write_varint64(body, 0ui64);
		functions::write_bool(body, false);
		functions::write_varint64(body, 0ui64);
		bedrock_packet.send(viewer, bedrock_move_player, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_player_c::despawn(structures::session_s& viewer, const structures::player_s& subject)
	{
		body.clear();
		functions::write_zigzag64(body, subject.entity_id);
		bedrock_packet.send(viewer, bedrock_remove_actor, body);

		body.clear();
		functions::write_varint(body, 1);
		functions::write_varint(body, static_cast<std::int32_t>(bedrock_list_remove));
		functions::write_u8(body, bedrock_list_action_remove);
		functions::write_u64_le(body, subject.uuid_high);
		functions::write_u64_le(body, subject.uuid_low);
		bedrock_packet.send(viewer, bedrock_player_list, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_player_c::send_actor_data(structures::session_s& session)
	{
		body.clear();
		functions::write_varint64(body, static_cast<std::uint64_t>(session.entity_id));
		write_actor_data(body, session.name, true);
		functions::write_varint(body, 0);
		functions::write_varint(body, 0);
		functions::write_varint64(body, 0ui64);
		bedrock_packet.send(session, bedrock_set_actor_data, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_player_c::write_actor_data(std::vector<std::uint8_t>& out, std::string_view name, bool self)
	{
		auto baseline{ functions::get_actor_baseline() };
		functions::write_varint(out, static_cast<std::int32_t>(std::count_if(baseline.begin(), baseline.end(), [self](const auto& row) { return (row.self || !self); })));

		for (const auto& row : baseline)
		{
			if (row.self || !self)
			{
				auto value{ self ? row.self_integer : row.integer };

				functions::write_varint(out, row.id);
				functions::write_varint(out, row.kind);
				functions::write_u8(out, row.kind);

				switch (row.kind)
				{
				case bedrock_data_byte:
					functions::write_u8(out, static_cast<std::uint8_t>(value));
					break;

				case bedrock_data_short:
					functions::write_u16_le(out, static_cast<std::uint16_t>(value));
					break;

				case bedrock_data_int:
					functions::write_zigzag32(out, static_cast<std::int32_t>(value));
					break;

				case bedrock_data_float:
					functions::write_f32_le(out, row.x);
					break;

				case bedrock_data_string:
					functions::write_string(out, row.id == bedrock_actor_name ? name : std::string_view{ row.text ? row.text : "" });
					break;

				case bedrock_data_compound:
					functions::write_bytes(out, bedrock_empty_compound, sizeof(bedrock_empty_compound));
					break;

				case bedrock_data_position:
					functions::write_zigzag32(out, static_cast<std::int32_t>(row.x));
					functions::write_zigzag32(out, static_cast<std::int32_t>(row.y));
					functions::write_zigzag32(out, static_cast<std::int32_t>(row.z));
					break;

				case bedrock_data_long:
					functions::write_zigzag64(out, value);
					break;

				case bedrock_data_vector:
					functions::write_f32_le(out, row.x);
					functions::write_f32_le(out, row.y);
					functions::write_f32_le(out, row.z);
					break;
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_player_c::write_skin(std::vector<std::uint8_t>& out, const structures::skin_s& skin)
	{
		functions::write_string(out, skin.id);
		functions::write_string(out, "");
		functions::write_string(out, skin.patch);
		functions::write_u32_le(out, skin.width);
		functions::write_u32_le(out, skin.height);
		functions::write_varint(out, static_cast<std::int32_t>(skin.image.size()));
		functions::write_bytes(out, skin.image.data(), skin.image.size());
		functions::write_varint(out, 0);
		functions::write_u32_le(out, 0u);
		functions::write_u32_le(out, 0u);
		functions::write_varint(out, 0);
		functions::write_string(out, skin.geometry);
		functions::write_string(out, skin.engine);
		functions::write_string(out, "");
		functions::write_string(out, "");
		functions::write_string(out, skin.id);
		functions::write_u8(out, skin.arm);
		functions::write_u32_le(out, 0u);
		functions::write_varint(out, 0);
		functions::write_varint(out, 0);
		functions::write_bool(out, skin.premium);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_bool(out, false);
		functions::write_string(out, bedrock_skin_trusted);
		functions::write_string(out, "");
	}
}

//=====================================================================================
