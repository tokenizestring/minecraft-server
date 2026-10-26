
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
		for (auto other : players)
		{
			spawn(*other, player, true);
			spawn(player, *other, true);
		}

		player.spawned = true;
		player.moved = false;
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

			for (auto other : players)
			{
				despawn(*other, player);
			}

			utils::log.write(color_play, "[world](remove) '%s' entity %d, %zu players", player.name.c_str(), player.entity_id, players.size());
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

			for (auto viewer : players | std::views::filter([subject](auto other) { return other != subject; }))
			{
				move(*viewer, *subject);
			}
		}

		dirty.clear();
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
	/*
	//=====================================================================================
	*/
	const std::vector<structures::player_s*>& world_c::get_players() const
	{
		return players;
	}
}

//=====================================================================================
