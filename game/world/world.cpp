
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	world_c world;

	std::int32_t world_c::allocate_entity_id()
	{
		return ++next_entity_id;
	}
	/*
	//=====================================================================================
	*/
	void world_c::add(structures::player_s& player)
	{
		player.spawned = true;
		player.moved = false;
		join_world(player);
		players.push_back(&player);
		spawn(player, player, false);

		utils::log.write(color_play, "[world](add) '%s' entity %d, %zu players", player.name.c_str(), player.entity_id, players.size());
	}
	/*
	//=====================================================================================
	*/
	void world_c::remove(structures::player_s& player)
	{
		if (player.spawned)
		{
			player.spawned = false;
			std::erase(players, &player);
			std::erase(dirty, &player);

			for (auto other : players | std::views::filter([&player](auto candidate) { return candidate->world == player.world; }))
			{
				despawn(*other, player);
			}

			utils::log.write(color_play, "[world](remove) '%s' entity %d, %zu players", player.name.c_str(), player.entity_id, players.size());
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::change(structures::player_s& player, structures::world_e target)
	{
		if (player.spawned && player.world != target)
		{
			std::erase(players, &player);
			leave_world(player);
			player.world = target;
			join_world(player);
			players.push_back(&player);

			utils::log.write(color_play, "[world](change) '%s' to '%s'", player.name.c_str(), world_names[target]);
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::moved(structures::player_s& player)
	{
		if (player.spawned && !player.moved)
		{
			player.moved = true;
			dirty.push_back(&player);
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::tick()
	{
		for (auto subject : dirty)
		{
			subject->moved = false;

			if (subject->world == structures::world_spawn)
			{
				npc.look(*subject);
			}

			for (auto viewer : players | std::views::filter([subject](auto other) { return other != subject && other->world == subject->world; }))
			{
				move(*viewer, *subject);
			}
		}

		dirty.clear();
		age += 1u;
		smp_time += config.smp_day_cycle;

		if (age % time_sync_ticks == 0u)
		{
			for (auto player : players)
			{
				if (player->edition == structures::edition_java && player->npc == false)
				{
					java_play.send_time(*static_cast<structures::client_s*>(player->link));
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::set_spawn(structures::world_e target, const structures::spawn_s& spawn)
	{
		spawns[target] = spawn;
	}
	/*
	//=====================================================================================
	*/
	const structures::spawn_s& world_c::get_spawn(structures::world_e target) const
	{
		return spawns[target];
	}
	/*
	//=====================================================================================
	*/
	const std::vector<structures::player_s*>& world_c::get_players() const
	{
		return players;
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t world_c::get_time(structures::world_e target) const
	{
		return target == structures::world_smp ? smp_start_time + smp_time : config.spawn_time;
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t world_c::get_age() const
	{
		return age;
	}
	/*
	//=====================================================================================
	*/
	bool world_c::is_frozen(structures::world_e target) const
	{
		return target != structures::world_smp || config.smp_day_cycle == 0u;
	}
	/*
	//=====================================================================================
	*/
	void world_c::join_world(structures::player_s& player)
	{
		for (auto other : players | std::views::filter([&player](auto candidate) { return candidate->world == player.world; }))
		{
			spawn(*other, player, true);
			spawn(player, *other, true);
		}

		if (player.world == structures::world_spawn)
		{
			npc.show(player);
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::leave_world(structures::player_s& player)
	{
		for (auto other : players | std::views::filter([&player](auto candidate) { return candidate->world == player.world; }))
		{
			despawn(*other, player);
			despawn(player, *other);
		}

		if (player.world == structures::world_spawn)
		{
			npc.hide(player);
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::spawn(structures::player_s& viewer, const structures::player_s& subject, bool entity)
	{
		if (viewer.edition == structures::edition_bedrock)
		{
			bedrock_player.spawn(*static_cast<structures::session_s*>(viewer.link), subject, entity);
		}

		else
		{
			java_player.spawn(*static_cast<structures::client_s*>(viewer.link), subject, entity);
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::despawn(structures::player_s& viewer, const structures::player_s& subject)
	{
		if (viewer.edition == structures::edition_bedrock)
		{
			bedrock_player.despawn(*static_cast<structures::session_s*>(viewer.link), subject);
		}

		else
		{
			java_player.despawn(*static_cast<structures::client_s*>(viewer.link), subject);
		}
	}
	/*
	//=====================================================================================
	*/
	void world_c::move(structures::player_s& viewer, const structures::player_s& subject)
	{
		if (viewer.edition == structures::edition_bedrock)
		{
			bedrock_player.move(*static_cast<structures::session_s*>(viewer.link), subject);
		}

		else
		{
			java_player.move(*static_cast<structures::client_s*>(viewer.link), subject);
		}
	}
}

//=====================================================================================
