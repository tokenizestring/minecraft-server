
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	drops_c drops;

	void drops_c::load()
	{
		std::vector<std::uint8_t> file{};
		std::uint64_t seed{};

		if (auto handle{ std::fopen(functions::get_local_path(ground_path).c_str(), "rb") }; handle)
		{
			std::fseek(handle, 0, SEEK_END);
			file.resize(static_cast<std::size_t>(std::ftell(handle)));
			std::fseek(handle, 0, SEEK_SET);
			file.resize(std::fread(file.data(), 1u, file.size(), handle));
			std::fclose(handle);
		}

		structures::reader_s in{ file.data(), file.size(), 0u, true };
		auto magic{ functions::read_u32_le(in) };
		auto version{ functions::read_u32_le(in) };
		functions::read_raw(in, &seed, sizeof(seed));
		auto count{ functions::read_u32_le(in) };
		auto usable{ in.good() && magic == ground_magic && version == ground_version && seed == config.smp_seed };

		for (auto index{ 0u }; usable && index < count && in.good(); ++index)
		{
			std::array<std::double_t, 3u> position{};
			auto item{ items.find_name(functions::read_string(in, max_kind_name)) };
			auto amount{ functions::read_u8(in) };
			auto age{ functions::read_u32_le(in) };
			functions::read_raw(in, position.data(), sizeof(position));

			if (in.good() && spawn(structures::world_smp, { item, amount }, position[0], position[1], position[2]))
			{
				ground.back().age = age;
			}
		}

		utils::log.write(color_server, "[drops](load) %zu items on the ground", ground.size());
	}
	/*
	//=====================================================================================
	*/
	void drops_c::save() const
	{
		std::vector<std::uint8_t> out{};
		functions::write_u32_le(out, ground_magic);
		functions::write_u32_le(out, ground_version);
		functions::write_u64_le(out, config.smp_seed);
		functions::write_u32_le(out, static_cast<std::uint32_t>(ground.size()));

		for (const auto& drop : ground)
		{
			functions::write_string(out, items.get_name(drop.stack.item));
			functions::write_u8(out, drop.stack.count);
			functions::write_u32_le(out, drop.age);
			functions::write_bytes(out, &drop.x, sizeof(drop.x));
			functions::write_bytes(out, &drop.y, sizeof(drop.y));
			functions::write_bytes(out, &drop.z, sizeof(drop.z));
		}

		if (auto file{ std::fopen(functions::get_local_path(ground_path).c_str(), "wb") }; file)
		{
			std::fwrite(out.data(), 1u, out.size(), file);
			std::fclose(file);
			utils::log.write(color_server, "[drops](save) %zu items on the ground", ground.size());
		}
	}
	/*
	//=====================================================================================
	*/
	bool drops_c::spawn(structures::world_e target, const structures::item_stack_s& stack, std::double_t x, std::double_t y, std::double_t z)
	{
		auto room{ target == structures::world_smp && stack.item && stack.count && ground.size() < drop_cap };

		if (room)
		{
			sequence += 1u;

			structures::drop_s drop{};
			drop.entity_id = world.allocate_entity_id();
			drop.uuid_high = (0x49544Dui64 << 40u) | sequence;
			drop.uuid_low = 0x8000000000000000ui64 | ((sequence * 0x9E3779B97F4A7C15ui64) >> 1u);
			drop.stack = stack;
			drop.world = target;
			drop.x = x;
			drop.y = y;
			drop.z = z;
			drop.sent_x = x;
			drop.sent_y = y;
			drop.sent_z = z;
			ground.push_back(drop);
			show(ground.back());
		}

		return room;
	}
	/*
	//=====================================================================================
	*/
	bool drops_c::toss(structures::player_s& thrower, const structures::item_stack_s& stack)
	{
		auto yaw{ static_cast<std::double_t>(thrower.yaw) * std::numbers::pi / 180.0 };
		auto pitch{ static_cast<std::double_t>(thrower.pitch) * std::numbers::pi / 180.0 };
		auto look_x{ -std::sin(yaw) * std::cos(pitch) };
		auto look_y{ -std::sin(pitch) };
		auto look_z{ std::cos(yaw) * std::cos(pitch) };
		auto thrown{ spawn(thrower.world, stack, thrower.x + look_x * drop_throw_reach, thrower.y + drop_throw_height, thrower.z + look_z * drop_throw_reach) };

		if (thrown)
		{
			auto& drop{ ground.back() };
			drop.velocity_x = look_x * drop_throw_speed;
			drop.velocity_y = look_y * drop_throw_speed + drop_throw_lift;
			drop.velocity_z = look_z * drop_throw_speed;
			drop.thrower = thrower.entity_id;
			drop.held_back = drop_release_ticks;
		}

		return thrown;
	}
	/*
	//=====================================================================================
	*/
	void drops_c::clear(structures::world_e target)
	{
		for (auto index{ ground.size() }; index > 0u; --index)
		{
			if (ground[index - 1u].world == target)
			{
				erase(index - 1u);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void drops_c::tick()
	{
		clock += 1u;
		build_grid();

		for (auto index{ ground.size() }; index > 0u; --index)
		{
			auto& drop{ ground[index - 1u] };
			drop.age += 1u;

			std::double_t distance{};
			auto* target{ drop.age >= drop_pickup_delay ? find_target(drop, distance) : nullptr };
			auto expired{ drop.age > drop_lifetime_ticks || drop.y < static_cast<std::double_t>(world_min_y - drop_floor_search) };
			auto taken{ expired == false && target && distance <= drop_pickup_radius && collect(index - 1u, *target) };

			if (expired)
			{
				erase(index - 1u);
			}

			else if (taken == false && target)
			{
				pull(drop, *target, distance);
				broadcast_move(drop);
			}

			else if (taken == false)
			{
				step(drop);
				broadcast_move(drop);
			}
		}

		if (clock % drop_view_ticks == 0u)
		{
			refresh();
		}

		if (clock % static_cast<std::uint32_t>(autosave_ms / tick_ms) == 0u && ground.size())
		{
			save();
		}
	}
	/*
	//=====================================================================================
	*/
	void drops_c::build_grid()
	{
		for (auto& cells : nearby)
		{
			cells.clear();
		}

		for (auto player : world.get_players())
		{
			if (player->edition == structures::edition_java)
			{
				nearby[player->world][edits_c::get_key(get_chunk(player->x), get_chunk(player->z))].push_back(player);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void drops_c::step(structures::drop_s& drop)
	{
		auto* owner{ drop.thrower ? find_player(drop.thrower) : nullptr };
		auto gap_x{ owner ? owner->x - drop.x : 0.0 };
		auto gap_y{ owner ? owner->y + drop_body_height - drop.y : 0.0 };
		auto gap_z{ owner ? owner->z - drop.z : 0.0 };
		auto span{ std::sqrt(gap_x * gap_x + gap_y * gap_y + gap_z * gap_z) };

		drop.held_back = drop.held_back && gap_x * gap_x + gap_z * gap_z <= drop_release_distance * drop_release_distance ? drop.held_back - 1u : 0u;
		drop.returning = owner && owner->world == drop.world && span <= drop_return_radius && (drop.returning || (drop.resting && drop.age >= drop_return_ticks));

		if (drop.returning && span > drop_move_epsilon)
		{
			auto pace{ std::min(drop_return_speed, span) };
			drop.x += gap_x / span * pace;
			drop.y += gap_y / span * pace;
			drop.z += gap_z / span * pace;
			drop.resting = false;
		}

		else if (drop.returning == false && drop.resting == false)
		{
			fall(drop);
		}
	}
	/*
	//=====================================================================================
	*/
	void drops_c::pull(structures::drop_s& drop, const structures::player_s& target, std::double_t distance)
	{
		auto span{ std::max(distance, drop_move_epsilon) };
		auto speed{ drop_magnet_speed * std::max(0.35, 1.0 - distance / drop_magnet_radius) * 2.4 };
		auto pace{ std::min(speed, span) };

		drop.x += (target.x - drop.x) / span * pace;
		drop.y += (target.y + drop_body_height - drop.y) / span * pace;
		drop.z += (target.z - drop.z) / span * pace;
		drop.resting = false;
	}
	/*
	//=====================================================================================
	*/
	void drops_c::fall(structures::drop_s& drop)
	{
		auto next_x{ drop.x + drop.velocity_x };
		auto next_z{ drop.z + drop.velocity_z };
		drop.velocity_y -= drop_gravity;

		if (is_solid(drop.world, next_x, drop.y + 0.1, drop.z))
		{
			drop.velocity_x *= drop_bounce;
		}

		else
		{
			drop.x = next_x;
		}

		if (is_solid(drop.world, drop.x, drop.y + 0.1, next_z))
		{
			drop.velocity_z *= drop_bounce;
		}

		else
		{
			drop.z = next_z;
		}

		drop.y += drop.velocity_y;
		auto floor{ get_floor(drop.world, drop.x, drop.y + 0.5, drop.z) };
		auto landed{ drop.y <= floor };
		drop.y = landed ? floor : drop.y;
		drop.velocity_y = landed ? 0.0 : drop.velocity_y;
		drop.velocity_x *= landed ? drop_ground_drag : drop_air_drag;
		drop.velocity_z *= landed ? drop_ground_drag : drop_air_drag;

		if (std::fabs(drop.velocity_x) + std::fabs(drop.velocity_y) + std::fabs(drop.velocity_z) < drop_rest_speed && drop.y <= floor + 0.05)
		{
			drop.velocity_x = 0.0;
			drop.velocity_z = 0.0;
			drop.resting = true;
		}
	}
	/*
	//=====================================================================================
	*/
	bool drops_c::collect(std::size_t index, structures::player_s& target)
	{
		auto& drop{ ground[index] };
		auto left{ inventory.add(target, drop.stack.item, drop.stack.count) };
		auto taken{ drop.stack.count - left };

		if (taken)
		{
			for (auto viewer : get_viewers(drop))
			{
				java_drops.collect(*viewer, drop, target.entity_id, taken);
			}

			drop.stack.count = static_cast<std::uint8_t>(left);
			java_inventory.send_all(*static_cast<structures::client_s*>(target.link));
		}

		if (taken && left == 0u)
		{
			erase(index);
		}

		else if (taken)
		{
			for (auto viewer : get_viewers(drop))
			{
				java_drops.update(*viewer, drop);
			}
		}

		return taken && left == 0u;
	}
	/*
	//=====================================================================================
	*/
	void drops_c::erase(std::size_t index)
	{
		auto entity_id{ ground[index].entity_id };

		for (auto player : world.get_players())
		{
			auto* client{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };

			if (client && client->seen_drops.erase(entity_id))
			{
				java_drops.remove(*client, entity_id);
			}
		}

		ground[index] = std::move(ground.back());
		ground.pop_back();
	}
	/*
	//=====================================================================================
	*/
	void drops_c::refresh()
	{
		std::unordered_map<std::int32_t, std::size_t> by_id{};
		std::array<std::unordered_map<std::uint64_t, std::vector<std::size_t>>, structures::world_count> cells{};

		for (auto index{ 0u }; index < ground.size(); ++index)
		{
			by_id[ground[index].entity_id] = index;
			cells[ground[index].world][edits_c::get_key(get_chunk(ground[index].x), get_chunk(ground[index].z))].push_back(index);
		}

		for (auto player : world.get_players())
		{
			auto* client{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };
			auto chunk_x{ get_chunk(player->x) };
			auto chunk_z{ get_chunk(player->z) };

			if (client)
			{
				std::erase_if(client->seen_drops, [&](std::int32_t entity_id)
				{
					auto found{ by_id.find(entity_id) };
					auto keep{ found != by_id.end() && ground[found->second].world == player->world && std::abs(get_chunk(ground[found->second].x) - chunk_x) <= drop_view_chunks && std::abs(get_chunk(ground[found->second].z) - chunk_z) <= drop_view_chunks };

					if (keep == false)
					{
						java_drops.remove(*client, entity_id);
					}

					return keep == false;
				});
			}

			for (auto offset_x{ -drop_view_chunks }; client && offset_x <= drop_view_chunks; ++offset_x)
			{
				for (auto offset_z{ -drop_view_chunks }; offset_z <= drop_view_chunks; ++offset_z)
				{
					auto cell{ cells[player->world].find(edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z)) };

					for (auto index : cell != cells[player->world].end() ? cell->second : no_drops)
					{
						if (client->seen_drops.insert(ground[index].entity_id).second)
						{
							java_drops.spawn(*client, ground[index]);
						}
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void drops_c::show(const structures::drop_s& drop)
	{
		auto chunk_x{ get_chunk(drop.x) };
		auto chunk_z{ get_chunk(drop.z) };

		for (auto player : world.get_players())
		{
			auto* client{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };

			if (client && player->world == drop.world && std::abs(get_chunk(player->x) - chunk_x) <= drop_view_chunks && std::abs(get_chunk(player->z) - chunk_z) <= drop_view_chunks && client->seen_drops.insert(drop.entity_id).second)
			{
				java_drops.spawn(*client, drop);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void drops_c::broadcast_move(structures::drop_s& drop)
	{
		if (std::fabs(drop.x - drop.sent_x) >= drop_move_epsilon || std::fabs(drop.y - drop.sent_y) >= drop_move_epsilon || std::fabs(drop.z - drop.sent_z) >= drop_move_epsilon)
		{
			for (auto viewer : get_viewers(drop))
			{
				java_drops.move(*viewer, drop);
			}

			drop.sent_x = drop.x;
			drop.sent_y = drop.y;
			drop.sent_z = drop.z;
		}
	}
	/*
	//=====================================================================================
	*/
	structures::player_s* drops_c::find_target(const structures::drop_s& drop, std::double_t& distance) const
	{
		structures::player_s* best{ nullptr };
		auto chunk_x{ get_chunk(drop.x) };
		auto chunk_z{ get_chunk(drop.z) };
		distance = drop_magnet_radius;

		for (auto offset_x{ -1 }; offset_x <= 1; ++offset_x)
		{
			for (auto offset_z{ -1 }; offset_z <= 1; ++offset_z)
			{
				auto cell{ nearby[drop.world].find(edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z)) };

				for (auto player : cell != nearby[drop.world].end() ? cell->second : no_players)
				{
					auto gap_x{ player->x - drop.x };
					auto gap_y{ player->y + drop_body_height - drop.y };
					auto gap_z{ player->z - drop.z };
					auto span{ std::sqrt(gap_x * gap_x + gap_y * gap_y + gap_z * gap_z) };
					auto allowed{ player->gamemode != gamemode_spectator && (drop.held_back == 0u || player->entity_id != drop.thrower) };

					if (allowed && span < distance)
					{
						distance = span;
						best = player;
					}
				}
			}
		}

		return best;
	}
	/*
	//=====================================================================================
	*/
	structures::player_s* drops_c::find_player(std::int32_t entity_id) const
	{
		structures::player_s* found{ nullptr };

		for (auto player : world.get_players())
		{
			found = found == nullptr && player->entity_id == entity_id && player->edition == structures::edition_java ? player : found;
		}

		return found;
	}
	/*
	//=====================================================================================
	*/
	std::vector<structures::client_s*> drops_c::get_viewers(const structures::drop_s& drop) const
	{
		std::vector<structures::client_s*> viewers{};
		auto chunk_x{ get_chunk(drop.x) };
		auto chunk_z{ get_chunk(drop.z) };

		for (auto offset_x{ -drop_view_chunks - 1 }; offset_x <= drop_view_chunks + 1; ++offset_x)
		{
			for (auto offset_z{ -drop_view_chunks - 1 }; offset_z <= drop_view_chunks + 1; ++offset_z)
			{
				auto cell{ nearby[drop.world].find(edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z)) };

				for (auto player : cell != nearby[drop.world].end() ? cell->second : no_players)
				{
					if (auto* client{ static_cast<structures::client_s*>(player->link) }; client->seen_drops.contains(drop.entity_id))
					{
						viewers.push_back(client);
					}
				}
			}
		}

		return viewers;
	}
	/*
	//=====================================================================================
	*/
	bool drops_c::is_solid(structures::world_e target, std::double_t x, std::double_t y, std::double_t z)
	{
		auto kind{ lookup.get(target, static_cast<std::int32_t>(std::floor(x)), static_cast<std::int32_t>(std::floor(y)), static_cast<std::int32_t>(std::floor(z))) };
		return kind != kind_none && kind != structures::block_air && (kind >= placed_kind_base || blocks.is_fluid(static_cast<std::uint8_t>(kind)) == false);
	}
	/*
	//=====================================================================================
	*/
	std::double_t drops_c::get_floor(structures::world_e target, std::double_t x, std::double_t y, std::double_t z)
	{
		auto start{ static_cast<std::int32_t>(std::floor(y)) };
		auto floor{ static_cast<std::double_t>(start - drop_floor_search) };
		auto found{ false };

		for (auto block_y{ start }; found == false && block_y > start - drop_floor_search; --block_y)
		{
			found = is_solid(target, x, static_cast<std::double_t>(block_y), z);
			floor = found ? static_cast<std::double_t>(block_y) + 1.0 : floor;
		}

		return floor;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t drops_c::get_chunk(std::double_t value)
	{
		return static_cast<std::int32_t>(std::floor(value)) >> 4;
	}
}

//=====================================================================================
