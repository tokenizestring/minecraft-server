
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	mobs_c mobs;

	void mobs_c::tick()
	{
		clock += 1u;
		build_grid();

		for (auto player : world.get_players())
		{
			if (clock % mob_spawn_ticks == 0u && player->edition == structures::edition_java && player->world == structures::world_smp && player->gamemode != gamemode_spectator)
			{
				spawn_near(*player);
			}
		}

		for (auto index{ herd.size() }; index > 0u; --index)
		{
			auto& mob{ herd[index - 1u] };
			auto turn{ clock + static_cast<std::uint32_t>(mob.entity_id) };
			auto gone{ (mob.state == structures::mob_dying && mob.timer == 0u) || (turn % mob_despawn_ticks == 0u && is_near_player(mob) == false) };

			if (gone)
			{
				erase(index - 1u);
			}

			else
			{
				mob.timer = mob.timer ? mob.timer - 1u : 0u;
				mob.hurt = mob.hurt ? mob.hurt - 1u : 0u;

				if (turn % mob_think_ticks == 0u)
				{
					think(mob);
				}

				if (mob.state == structures::mob_walk || mob.state == structures::mob_flee)
				{
					walk(mob);
				}

				if (turn % mob_move_ticks == 0u)
				{
					broadcast(mob);
				}
			}
		}

		if (clock % mob_view_ticks == 0u)
		{
			refresh();
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::attack(structures::player_s& player, std::int32_t entity_id)
	{
		auto found{ std::find_if(herd.begin(), herd.end(), [entity_id](const auto& mob) { return mob.entity_id == entity_id; }) };
		auto gap_x{ found != herd.end() ? found->x - player.x : 0.0 };
		auto gap_z{ found != herd.end() ? found->z - player.z : 0.0 };
		auto close{ found != herd.end() && player.world == structures::world_smp && gap_x * gap_x + gap_z * gap_z <= mob_reach * mob_reach };

		if (close && found->state != structures::mob_dying && found->hurt == 0u)
		{
			auto& mob{ *found };
			const auto& held{ player.inventory[player.held_slot] };
			auto span{ std::max(std::sqrt(gap_x * gap_x + gap_z * gap_z), 0.01) };
			auto push_x{ mob.x + gap_x / span * mob_knockback };
			auto push_z{ mob.z + gap_z / span * mob_knockback };
			auto stand{ surface.get(static_cast<std::int32_t>(std::floor(push_x)), static_cast<std::int32_t>(std::floor(push_z))) };

			mob.health -= held.count ? items.get_damage(held.item) : fist_damage;
			mob.hurt = mob_hurt_ticks;
			vitals.exhaust(player, exhaustion_attack);

			for (auto viewer : get_viewers(mob))
			{
				java_mobs.hurt(*viewer, mob);
			}

			if (stand != surface_none && std::abs(static_cast<std::int32_t>(stand) + 1 - static_cast<std::int32_t>(std::floor(mob.y))) <= mob_max_climb)
			{
				mob.x = push_x;
				mob.z = push_z;
				mob.y = static_cast<std::double_t>(stand) + 1.0;
				mob.moved = true;
			}

			if (mob.health <= 0.0f)
			{
				play(mob, sound_death, false);
				kill(mob);
			}

			else
			{
				play(mob, sound_hurt, false);
				mob.state = structures::mob_flee;
				mob.timer = mob_flee_ticks;
				aim(mob, gap_x, gap_z, mob_wander_range * 2);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::forget(const structures::client_s& client)
	{
		for (auto& [key, list] : watchers)
		{
			std::erase(list, &client);
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::clear()
	{
		for (auto index{ herd.size() }; index > 0u; --index)
		{
			erase(index - 1u);
		}
	}
	/*
	//=====================================================================================
	*/
	std::size_t mobs_c::count() const
	{
		return herd.size();
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::build_grid()
	{
		players.clear();
		census.clear();

		for (auto player : world.get_players())
		{
			if (player->edition == structures::edition_java && player->world == structures::world_smp)
			{
				players[edits_c::get_key(get_chunk(player->x), get_chunk(player->z))].push_back(player);
			}
		}

		for (const auto& mob : clock % mob_spawn_ticks == 0u ? herd : no_herd)
		{
			census[edits_c::get_key(get_chunk(mob.x), get_chunk(mob.z))] += 1u;
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::spawn_near(const structures::player_s& player)
	{
		auto home_x{ get_chunk(player.x) };
		auto home_z{ get_chunk(player.z) };
		auto offset_x{ static_cast<std::int32_t>(roll(static_cast<std::uint32_t>(mob_spawn_far_chunks * 2 + 1))) - mob_spawn_far_chunks };
		auto offset_z{ static_cast<std::int32_t>(roll(static_cast<std::uint32_t>(mob_spawn_far_chunks * 2 + 1))) - mob_spawn_far_chunks };
		auto chunk_x{ home_x + offset_x };
		auto chunk_z{ home_z + offset_z };
		auto outside{ std::max(std::abs(offset_x), std::abs(offset_z)) >= mob_spawn_near_chunks };
		auto room{ herd.size() < config.mob_cap && count_near(home_x, home_z) < config.mob_density };

		if (outside && room && surface.has(chunk_x, chunk_z))
		{
			auto center_x{ chunk_x * 16 + static_cast<std::int32_t>(roll(16u)) };
			auto center_z{ chunk_z * 16 + static_cast<std::int32_t>(roll(16u)) };
			auto center{ surface.get(center_x, center_z) };
			auto kind{ static_cast<std::uint8_t>(roll(static_cast<std::uint32_t>(std::size(mob_names)))) };
			auto pack{ mob_pack_min + roll(mob_pack_max - mob_pack_min + 1u) };

			for (auto member{ 0u }; center != surface_none && member < pack; ++member)
			{
				auto x{ center_x + static_cast<std::int32_t>(roll(static_cast<std::uint32_t>(mob_pack_spread * 2 + 1))) - mob_pack_spread };
				auto z{ center_z + static_cast<std::int32_t>(roll(static_cast<std::uint32_t>(mob_pack_spread * 2 + 1))) - mob_pack_spread };
				auto stand{ surface.get(x, z) };

				if (stand != surface_none && std::abs(stand - center) <= mob_max_climb + 1)
				{
					spawn(kind, static_cast<std::double_t>(x) + 0.5, static_cast<std::double_t>(stand) + 1.0, static_cast<std::double_t>(z) + 0.5);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::spawn(std::uint8_t kind, std::double_t x, std::double_t y, std::double_t z)
	{
		sequence += 1u;

		structures::mob_s mob{};
		mob.entity_id = world.allocate_entity_id();
		mob.uuid_high = (0x4D4F42ui64 << 40u) | sequence;
		mob.uuid_low = 0x8000000000000000ui64 | ((sequence * 0x9E3779B97F4A7C15ui64) >> 1u);
		mob.kind = kind;
		mob.state = structures::mob_idle;
		mob.x = x;
		mob.y = y;
		mob.z = z;
		mob.sent_x = x;
		mob.sent_y = y;
		mob.sent_z = z;
		mob.target_x = x;
		mob.target_z = z;
		mob.yaw = static_cast<std::float_t>(roll(360u));
		mob.sent_yaw = functions::get_angle_byte(mob.yaw);
		mob.health = mob_health[kind];
		herd.push_back(mob);
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::think(structures::mob_s& mob)
	{
		auto angle{ static_cast<std::double_t>(roll(360u)) * std::numbers::pi / 180.0 };

		if (mob.state == structures::mob_flee && mob.timer == 0u)
		{
			mob.state = structures::mob_idle;
		}

		else if (mob.state == structures::mob_idle && roll(mob_wander_chance) == 0u)
		{
			aim(mob, std::cos(angle), std::sin(angle), mob_wander_range);
			mob.state = structures::mob_walk;
		}

		if (mob.state != structures::mob_dying && roll(mob_ambient_chance) == 0u)
		{
			play(mob, sound_ambient, true);
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::play(const structures::mob_s& mob, std::uint32_t which, bool nearby)
	{
		for (auto viewer : get_viewers(mob))
		{
			auto gap_x{ viewer->player.x - mob.x };
			auto gap_z{ viewer->player.z - mob.z };

			if (nearby == false || gap_x * gap_x + gap_z * gap_z <= mob_sound_range * mob_sound_range)
			{
				java_mobs.sound(*viewer, mob, which);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::walk(structures::mob_s& mob)
	{
		auto gap_x{ mob.target_x - mob.x };
		auto gap_z{ mob.target_z - mob.z };
		auto span{ std::sqrt(gap_x * gap_x + gap_z * gap_z) };
		auto pace{ std::min(mob.state == structures::mob_flee ? mob_flee_speed : mob_speed[mob.kind], span) };
		auto next_x{ span > mob_arrive ? mob.x + gap_x / span * pace : mob.x };
		auto next_z{ span > mob_arrive ? mob.z + gap_z / span * pace : mob.z };
		auto stand{ surface.get(static_cast<std::int32_t>(std::floor(next_x)), static_cast<std::int32_t>(std::floor(next_z))) };
		auto rise{ static_cast<std::int32_t>(stand) + 1 - static_cast<std::int32_t>(std::floor(mob.y)) };
		auto open{ stand != surface_none && rise <= mob_max_climb && -rise <= mob_max_drop };

		if (span <= mob_arrive && mob.state == structures::mob_flee)
		{
			aim(mob, gap_x, gap_z, mob_wander_range);
		}

		else if (span <= mob_arrive)
		{
			mob.state = structures::mob_idle;
		}

		else if (open)
		{
			mob.x = next_x;
			mob.z = next_z;
			mob.y = static_cast<std::double_t>(stand) + 1.0;
			mob.yaw = static_cast<std::float_t>(std::atan2(-gap_x, gap_z) * 180.0 / std::numbers::pi);
			mob.moved = true;
		}

		else if (mob.state == structures::mob_flee)
		{
			aim(mob, -gap_z, gap_x, mob_wander_range);
		}

		else
		{
			mob.state = structures::mob_idle;
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::kill(structures::mob_s& mob)
	{
		auto meat{ items.find_name(mob_meat[mob.kind]) };
		auto extra{ items.find_name(mob_extra[mob.kind]) };
		auto meat_count{ 1u + roll(mob_meat_max[mob.kind]) };
		auto extra_count{ roll(mob_extra_max[mob.kind] + 1u) };

		mob.state = structures::mob_dying;
		mob.timer = mob_dying_ticks;

		for (auto viewer : get_viewers(mob))
		{
			java_mobs.die(*viewer, mob);
		}

		drops.spawn(structures::world_smp, { meat, static_cast<std::uint8_t>(meat_count) }, mob.x, mob.y + 0.5, mob.z);

		if (extra_count)
		{
			drops.spawn(structures::world_smp, { extra, static_cast<std::uint8_t>(extra_count) }, mob.x, mob.y + 0.5, mob.z);
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::erase(std::size_t index)
	{
		auto entity_id{ herd[index].entity_id };

		for (auto player : world.get_players())
		{
			auto* client{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };

			if (client && client->seen_mobs.erase(entity_id))
			{
				java_mobs.remove(*client, entity_id);
			}
		}

		herd[index] = std::move(herd.back());
		herd.pop_back();
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::refresh()
	{
		std::unordered_map<std::int32_t, std::size_t> by_id{};
		std::unordered_map<std::uint64_t, std::vector<std::size_t>> cells{};

		for (auto index{ 0u }; index < herd.size(); ++index)
		{
			by_id[herd[index].entity_id] = index;
			cells[edits_c::get_key(get_chunk(herd[index].x), get_chunk(herd[index].z))].push_back(index);
		}

		watchers.clear();

		for (auto player : world.get_players())
		{
			auto* client{ player->edition == structures::edition_java ? static_cast<structures::client_s*>(player->link) : nullptr };
			auto chunk_x{ get_chunk(player->x) };
			auto chunk_z{ get_chunk(player->z) };
			auto smp{ player->world == structures::world_smp };

			for (auto offset_x{ -mob_view_chunks - 1 }; client && smp && offset_x <= mob_view_chunks + 1; ++offset_x)
			{
				for (auto offset_z{ -mob_view_chunks - 1 }; offset_z <= mob_view_chunks + 1; ++offset_z)
				{
					watchers[edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z)].push_back(client);
				}
			}

			if (client)
			{
				std::erase_if(client->seen_mobs, [&](std::int32_t entity_id)
				{
					auto found{ by_id.find(entity_id) };
					auto keep{ smp && found != by_id.end() && std::abs(get_chunk(herd[found->second].x) - chunk_x) <= mob_view_chunks && std::abs(get_chunk(herd[found->second].z) - chunk_z) <= mob_view_chunks };

					if (keep == false)
					{
						java_mobs.remove(*client, entity_id);
					}

					return keep == false;
				});
			}

			for (auto offset_x{ -mob_view_chunks }; client && smp && offset_x <= mob_view_chunks; ++offset_x)
			{
				for (auto offset_z{ -mob_view_chunks }; offset_z <= mob_view_chunks; ++offset_z)
				{
					auto cell{ cells.find(edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z)) };

					for (auto index : cell != cells.end() ? cell->second : no_mobs)
					{
						if (herd[index].state != structures::mob_dying && client->seen_mobs.insert(herd[index].entity_id).second)
						{
							java_mobs.spawn(*client, herd[index]);
						}
					}
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::broadcast(structures::mob_s& mob)
	{
		auto yaw{ functions::get_angle_byte(mob.yaw) };
		auto turned{ yaw != mob.sent_yaw };

		if (mob.moved || turned)
		{
			for (auto viewer : get_viewers(mob))
			{
				java_mobs.move(*viewer, mob, mob.moved, turned);
			}

			mob.sent_x = mob.x;
			mob.sent_y = mob.y;
			mob.sent_z = mob.z;
			mob.sent_yaw = yaw;
			mob.moved = false;
		}
	}
	/*
	//=====================================================================================
	*/
	void mobs_c::aim(structures::mob_s& mob, std::double_t direction_x, std::double_t direction_z, std::int32_t range)
	{
		auto span{ std::max(std::sqrt(direction_x * direction_x + direction_z * direction_z), 0.01) };
		auto reach{ static_cast<std::double_t>(range / 2 + static_cast<std::int32_t>(roll(static_cast<std::uint32_t>(range / 2 + 1)))) };

		mob.target_x = mob.x + direction_x / span * reach;
		mob.target_z = mob.z + direction_z / span * reach;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t mobs_c::count_near(std::int32_t chunk_x, std::int32_t chunk_z) const
	{
		auto total{ 0u };

		for (auto offset_x{ -mob_count_chunks }; offset_x <= mob_count_chunks; ++offset_x)
		{
			for (auto offset_z{ -mob_count_chunks }; offset_z <= mob_count_chunks; ++offset_z)
			{
				auto cell{ census.find(edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z)) };
				total += cell != census.end() ? cell->second : 0u;
			}
		}

		return total;
	}
	/*
	//=====================================================================================
	*/
	bool mobs_c::is_near_player(const structures::mob_s& mob) const
	{
		auto chunk_x{ get_chunk(mob.x) };
		auto chunk_z{ get_chunk(mob.z) };
		auto close{ false };

		for (auto offset_x{ -mob_despawn_chunks }; close == false && offset_x <= mob_despawn_chunks; ++offset_x)
		{
			for (auto offset_z{ -mob_despawn_chunks }; close == false && offset_z <= mob_despawn_chunks; ++offset_z)
			{
				close = players.contains(edits_c::get_key(chunk_x + offset_x, chunk_z + offset_z));
			}
		}

		return close;
	}
	/*
	//=====================================================================================
	*/
	std::vector<structures::client_s*> mobs_c::get_viewers(const structures::mob_s& mob) const
	{
		std::vector<structures::client_s*> viewers{};
		auto cell{ watchers.find(edits_c::get_key(get_chunk(mob.x), get_chunk(mob.z))) };

		for (auto client : cell != watchers.end() ? cell->second : no_clients)
		{
			if (client->seen_mobs.contains(mob.entity_id))
			{
				viewers.push_back(client);
			}
		}

		return viewers;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t mobs_c::roll(std::uint32_t range)
	{
		random = random ? random : functions::make_seed();
		random ^= random << 13u;
		random ^= random >> 7u;
		random ^= random << 17u;
		return range ? static_cast<std::uint32_t>(random % range) : 0u;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t mobs_c::get_chunk(std::double_t value)
	{
		return static_cast<std::int32_t>(std::floor(value)) >> 4;
	}
}

//=====================================================================================
