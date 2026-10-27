
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	friends_c friends;

	void friends_c::click(structures::player_s& player, std::string_view name, bool remove) const
	{
		auto* target{ profile_c::find_online(name) };
		auto mine{ has(player.friends, name) };
		auto asked{ has(player.requests, name) };
		auto label{ std::string{ name } };

		if (name == player.name)
		{
			chat.notify(player, "that is you");
		}

		else if (mine && remove)
		{
			unlink(player, name);
			chat.notify(player, label + " is no longer your friend");
		}

		else if (mine)
		{
			chat.notify(player, label + " is your friend, right click to remove");
		}

		else if (asked && player.friends.size() < max_friends)
		{
			link(player, name);
			chat.notify(player, "you and " + label + " are now friends");
		}

		else if (asked)
		{
			chat.notify(player, "you have too many friends");
		}

		else if (target && has(target->requests, player.name))
		{
			chat.notify(player, "you already asked " + label);
		}

		else if (target && target->requests.size() < max_requests)
		{
			target->requests.push_back(player.name);
			chat.notify(*target, player.name + " wants to be your friend, open friends to accept");
			chat.notify(player, "friend request sent to " + label);
		}

		else if (target)
		{
			chat.notify(player, label + " has too many requests");
		}

		else
		{
			chat.notify(player, label + " is not online");
		}
	}
	/*
	//=====================================================================================
	*/
	void friends_c::joined(structures::player_s& player) const
	{
		auto online{ 0u };

		for (auto other : world.get_players())
		{
			if (other != &player && has(other->friends, player.name))
			{
				chat.notify(*other, "your friend " + player.name + " joined");
			}

			online += other != &player && has(player.friends, other->name) ? 1u : 0u;
		}

		if (online)
		{
			chat.notify(player, std::to_string(online) + (online == 1u ? " friend is online" : " friends are online"));
		}
	}
	/*
	//=====================================================================================
	*/
	std::vector<structures::social_entry_s> friends_c::list(const structures::player_s& player) const
	{
		std::vector<structures::social_entry_s> entries{};

		for (const auto& name : player.requests)
		{
			entries.push_back({ name, social_wants_you });
		}

		for (const auto& name : player.friends)
		{
			if (profile_c::find_online(name))
			{
				entries.push_back({ name, social_friend_online });
			}
		}

		for (auto other : world.get_players())
		{
			if (other != &player && has(player.friends, other->name) == false && has(player.requests, other->name) == false)
			{
				entries.push_back({ other->name, has(other->requests, player.name) ? social_requested : social_stranger });
			}
		}

		for (const auto& name : player.friends)
		{
			if (profile_c::find_online(name) == nullptr)
			{
				entries.push_back({ name, social_friend_offline });
			}
		}

		return entries;
	}
	/*
	//=====================================================================================
	*/
	void friends_c::link(structures::player_s& player, std::string_view name) const
	{
		auto* target{ profile_c::find_online(name) };
		auto add{ [&player](structures::player_s& other)
		{
			if (has(other.friends, player.name) == false)
			{
				other.friends.push_back(player.name);
			}

			std::erase(other.requests, player.name);
		} };

		player.friends.emplace_back(name);
		std::erase(player.requests, std::string{ name });
		profile.save(player);

		if (target)
		{
			add(*target);
			profile.save(*target);
			chat.notify(*target, "you and " + player.name + " are now friends");
		}

		else
		{
			profile.edit(name, add);
		}
	}
	/*
	//=====================================================================================
	*/
	void friends_c::unlink(structures::player_s& player, std::string_view name) const
	{
		auto* target{ profile_c::find_online(name) };
		auto drop{ [&player](structures::player_s& other) { std::erase(other.friends, player.name); } };

		std::erase(player.friends, std::string{ name });
		profile.save(player);

		if (target)
		{
			drop(*target);
			profile.save(*target);
		}

		else
		{
			profile.edit(name, drop);
		}
	}
	/*
	//=====================================================================================
	*/
	bool friends_c::has(const std::vector<std::string>& names, std::string_view name)
	{
		return std::find(names.begin(), names.end(), name) != names.end();
	}
}

//=====================================================================================
