
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_drops_c java_drops;

	void java_drops_c::spawn(structures::client_s& viewer, const structures::drop_s& drop)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_add_entity);
			functions::write_varint(packet, drop.entity_id);
			functions::write_u64(packet, drop.uuid_high);
			functions::write_u64(packet, drop.uuid_low);
			functions::write_varint(packet, viewer.layout.item_entity_type);
			functions::write_f64(packet, drop.x);
			functions::write_f64(packet, drop.y);
			functions::write_f64(packet, drop.z);
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, 0u);
			functions::write_varint(packet, 0);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_spawn_object);
			functions::write_varint(packet, drop.entity_id);
			functions::write_u8(packet, v1_8_object_item);
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(drop.x * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(drop.y * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(drop.z * v1_8_fixed_scale)));
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, 0u);
			functions::write_i32(packet, 1);
			functions::write_u16(packet, 0u);
			functions::write_u16(packet, 0u);
			functions::write_u16(packet, 0u);
		}

		network.send(viewer, packet);
		update(viewer, drop);
	}
	/*
	//=====================================================================================
	*/
	void java_drops_c::update(structures::client_s& viewer, const structures::drop_s& drop)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_entity_data);
			functions::write_varint(packet, drop.entity_id);
			functions::write_u8(packet, java_metadata_item_index);
			functions::write_varint(packet, java_metadata_item_type);
			java_inventory_c::write_item(packet, viewer.family, drop.stack);
			functions::write_u8(packet, java_metadata_end);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_entity_metadata);
			functions::write_varint(packet, drop.entity_id);
			functions::write_u8(packet, v1_8_metadata_item);
			java_inventory_c::write_item(packet, viewer.family, drop.stack);
			functions::write_u8(packet, v1_8_metadata_end);
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_drops_c::move(structures::client_s& viewer, const structures::drop_s& drop)
	{
		auto modern{ functions::is_modern(viewer.family) };
		auto scale{ modern ? java_move_scale : v1_8_fixed_scale };
		auto limit{ static_cast<std::int64_t>((modern ? java_move_limit : v1_8_move_limit) * scale) };
		auto step_x{ get_step(drop.sent_x, drop.x, scale) };
		auto step_y{ get_step(drop.sent_y, drop.y, scale) };
		auto step_z{ get_step(drop.sent_z, drop.z, scale) };
		auto relative{ std::abs(step_x) < limit && std::abs(step_y) < limit && std::abs(step_z) < limit };

		packet.clear();

		if (modern && relative)
		{
			functions::write_varint(packet, viewer.layout.play_out_move_entity_pos);
			functions::write_varint(packet, drop.entity_id);

			if (viewer.layout.packed_move)
			{
				functions::write_varint(packet, drop.resting ? 1 : 0);
			}

			functions::write_u16(packet, static_cast<std::uint16_t>(static_cast<std::int16_t>(step_x)));
			functions::write_u16(packet, static_cast<std::uint16_t>(static_cast<std::int16_t>(step_y)));
			functions::write_u16(packet, static_cast<std::uint16_t>(static_cast<std::int16_t>(step_z)));

			if (viewer.layout.packed_move == false)
			{
				functions::write_bool(packet, drop.resting);
			}
		}

		else if (modern)
		{
			functions::write_varint(packet, viewer.layout.play_out_teleport_entity);
			functions::write_varint(packet, drop.entity_id);
			functions::write_f64(packet, drop.x);
			functions::write_f64(packet, drop.y);
			functions::write_f64(packet, drop.z);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f64(packet, 0.0);
			functions::write_f32(packet, 0.0f);
			functions::write_f32(packet, 0.0f);
			functions::write_i32(packet, 0);
			functions::write_bool(packet, drop.resting);
		}

		else if (relative)
		{
			functions::write_varint(packet, v1_8_play_out_entity_move);
			functions::write_varint(packet, drop.entity_id);
			functions::write_u8(packet, static_cast<std::uint8_t>(static_cast<std::int8_t>(step_x)));
			functions::write_u8(packet, static_cast<std::uint8_t>(static_cast<std::int8_t>(step_y)));
			functions::write_u8(packet, static_cast<std::uint8_t>(static_cast<std::int8_t>(step_z)));
			functions::write_bool(packet, drop.resting);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_entity_teleport);
			functions::write_varint(packet, drop.entity_id);
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(drop.x * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(drop.y * v1_8_fixed_scale)));
			functions::write_i32(packet, static_cast<std::int32_t>(std::floor(drop.z * v1_8_fixed_scale)));
			functions::write_u8(packet, 0u);
			functions::write_u8(packet, 0u);
			functions::write_bool(packet, drop.resting);
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_drops_c::collect(structures::client_s& viewer, const structures::drop_s& drop, std::int32_t collector, std::uint32_t count)
	{
		packet.clear();

		if (functions::is_modern(viewer.family))
		{
			functions::write_varint(packet, viewer.layout.play_out_take_item);
			functions::write_varint(packet, drop.entity_id);
			functions::write_varint(packet, collector);
			functions::write_varint(packet, static_cast<std::int32_t>(count));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_collect);
			functions::write_varint(packet, drop.entity_id);
			functions::write_varint(packet, collector);
		}

		network.send(viewer, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_drops_c::remove(structures::client_s& viewer, std::int32_t entity_id)
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
	std::int64_t java_drops_c::get_step(std::double_t from, std::double_t to, std::double_t scale)
	{
		return static_cast<std::int64_t>(std::floor(to * scale)) - static_cast<std::int64_t>(std::floor(from * scale));
	}
}

//=====================================================================================
