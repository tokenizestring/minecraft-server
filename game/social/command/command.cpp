
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	command_c command;

	void command_c::run(structures::player_s& player, std::string_view line)
	{
		if (line.size() && line[0] == '/')
		{
			line.remove_prefix(1u);
		}

		auto name{ line.substr(0u, line.find(' ')) };
		auto argument{ line.size() > name.size() ? line.substr(name.size() + 1u) : std::string_view{} };

		utils::log.write(color_chat, "[command] '%s' /%.*s", player.name.c_str(), static_cast<std::int32_t>(line.size()), line.data());

		if (name == chat_command)
		{
			chat.toggle(player, argument);
		}

		else if ((name == gamemode_command || name == gamemode_alias) && config.is_operator(player.name))
		{
			set_gamemode(player, argument);
		}

		else if (name == gamemode_command || name == gamemode_alias)
		{
			chat.notify(player, "you are not an operator");
		}

		else if (name == smp_command)
		{
			travel(player, structures::world_smp);
		}

		else if (name == spawn_command)
		{
			travel(player, structures::world_spawn);
		}

		else if (name == rtp_command)
		{
			random_teleport(player);
		}

		else if (name == roll_command && config.is_operator(player.name))
		{
			roll(player);
		}

		else if (name == roll_command)
		{
			chat.notify(player, "you are not an operator");
		}

		else if (name == balance_command || name == balance_alias)
		{
			chat.notify(player, "balance " + std::to_string(player.balance));
		}

		else if (name == pay_command)
		{
			pay(player, argument);
		}

		else if (name == sell_command && player.edition == structures::edition_java && player.world == structures::world_smp)
		{
			market.sell(player, player.held_slot, argument);
			java_inventory.send_slot(*static_cast<structures::client_s*>(player.link), player.held_slot);
		}

		else if (name == sell_command && player.edition == structures::edition_java)
		{
			chat.notify(player, "your items live in the smp, sell from there");
		}

		else if (name == sell_command)
		{
			bedrock_market.open_sell(*static_cast<structures::session_s*>(player.link));
		}

		else if (name == buy_command && player.edition == structures::edition_java)
		{
			java_market.open(*static_cast<structures::client_s*>(player.link), argument);
		}

		else if (name == buy_command)
		{
			bedrock_market.open_buy(*static_cast<structures::session_s*>(player.link), argument);
		}

		else if (name == games_command)
		{
			hub.open(player, hub_menu_games);
		}

		else if (name == info_command)
		{
			hub.open(player, hub_menu_info);
		}

		else if (name == friends_command && player.edition == structures::edition_bedrock)
		{
			bedrock_menu.open_social(*static_cast<structures::session_s*>(player.link));
		}

		else if (name == friends_command)
		{
			java_social.open(*static_cast<structures::client_s*>(player.link));
		}

		else
		{
			chat.notify(player, "unknown command");
		}
	}
	/*
	//=====================================================================================
	*/
	void command_c::travel(structures::player_s& player, structures::world_e target)
	{
		if (player.edition == structures::edition_java)
		{
			java_play.switch_world(*static_cast<structures::client_s*>(player.link), target);
			chat.notify(player, std::string{ "welcome to " }.append(world_names[target]));
		}

		else
		{
			chat.notify(player, "the smp is coming to bedrock soon");
		}
	}
	/*
	//=====================================================================================
	*/
	void command_c::random_teleport(structures::player_s& player)
	{
		auto now{ functions::get_time_ms() };
		auto radius{ static_cast<std::int64_t>(config.rtp_radius) };
		auto found{ false };
		structures::spawn_s spot{};

		for (auto attempt{ 0u }; player.edition == structures::edition_java && now >= player.rtp_ms && found == false && attempt < rtp_tries; ++attempt)
		{
			auto roll{ functions::make_seed() };
			auto x{ static_cast<std::int32_t>(static_cast<std::int64_t>(roll % static_cast<std::uint64_t>(radius * 2 + 1)) - radius) };
			auto z{ static_cast<std::int32_t>(static_cast<std::int64_t>((roll >> 32u) % static_cast<std::uint64_t>(radius * 2 + 1)) - radius) };
			std::uint16_t kind{};

			if (lookup.is_land(x, z))
			{
				auto top{ lookup.get_top(x, z, kind) };
				found = top > gen_sea_level && kind != kind_none && (kind >= placed_kind_base || blocks.is_fluid(static_cast<std::uint8_t>(kind)) == false);
				spot = { static_cast<std::double_t>(x) + 0.5, static_cast<std::double_t>(top + 1), static_cast<std::double_t>(z) + 0.5, player.yaw };
			}
		}

		if (player.edition == structures::edition_bedrock)
		{
			chat.notify(player, "the smp is coming to bedrock soon");
		}

		else if (now < player.rtp_ms)
		{
			chat.notify(player, "wait " + std::to_string((player.rtp_ms - now + 999u) / 1000u) + "s before teleporting again");
		}

		else if (found)
		{
			player.rtp_ms = now + rtp_cooldown_ms;
			java_play.switch_world(*static_cast<structures::client_s*>(player.link), structures::world_smp, spot);
			chat.notify(player, "teleported to " + std::to_string(static_cast<std::int32_t>(spot.x)) + " " + std::to_string(static_cast<std::int32_t>(spot.z)));
			utils::log.write(color_play, "[world](rtp) '%s' to %.0f %.0f %.0f", player.name.c_str(), spot.x, spot.y, spot.z);
		}

		else
		{
			chat.notify(player, "no safe spot found, try again");
		}
	}
	/*
	//=====================================================================================
	*/
	void command_c::roll(structures::player_s& player)
	{
		config.smp_seed = functions::make_seed();
		config.store();
		generator.set_seed(config.smp_seed);
		edits.clear(structures::world_smp);
		drops.clear(structures::world_smp);
		mobs.clear();
		surface.clear();
		java_chunk.reset_world(structures::world_smp);
		world.set_spawn(structures::world_smp, generator.get_spawn());

		std::vector<structures::player_s*> travellers{};

		for (auto other : world.get_players())
		{
			if (other->world == structures::world_smp && other->edition == structures::edition_java)
			{
				travellers.push_back(other);
			}
		}

		for (auto other : travellers)
		{
			java_play.switch_world(*static_cast<structures::client_s*>(other->link), structures::world_smp);
			chat.notify(*other, "the smp map was rolled");
		}

		utils::log.write(color_play, "[world](roll) '%s' new smp seed %llu", player.name.c_str(), config.smp_seed);
		chat.notify(player, "smp rolled, new seed " + std::to_string(config.smp_seed));
	}
	/*
	//=====================================================================================
	*/
	void command_c::pay(structures::player_s& player, std::string_view argument)
	{
		auto space{ argument.rfind(' ') };
		auto target_name{ space != std::string_view::npos ? argument.substr(0u, space) : std::string_view{} };
		auto* target{ profile_c::find_online(target_name) };
		std::int64_t amount{};
		auto [end, error]{ std::from_chars(argument.data() + (space != std::string_view::npos ? space + 1u : 0u), argument.data() + argument.size(), amount) };

		if (target && target != &player && error == std::errc{} && amount > 0 && amount <= player.balance)
		{
			player.balance -= amount;
			target->balance += amount;
			profile.save(player);
			profile.save(*target);
			chat.notify(player, "paid " + target->name + " " + std::to_string(amount));
			chat.notify(*target, player.name + " paid you " + std::to_string(amount));
			utils::log.write(color_play, "[market](pay) '%s' paid '%s' %lld", player.name.c_str(), target->name.c_str(), amount);
		}

		else if (target && target != &player && error == std::errc{} && amount > 0)
		{
			chat.notify(player, "you only have " + std::to_string(player.balance));
		}

		else if (target_name.size() && target == nullptr)
		{
			chat.notify(player, "that player is not online");
		}

		else
		{
			chat.notify(player, "use /pay <name> <amount>");
		}
	}
	/*
	//=====================================================================================
	*/
	void command_c::set_gamemode(structures::player_s& player, std::string_view argument)
	{
		auto mode{ static_cast<std::uint32_t>(std::size(gamemode_arguments)) };

		for (auto index{ 0u }; index < std::size(gamemode_arguments); ++index)
		{
			if (argument == gamemode_arguments[index])
			{
				mode = index % static_cast<std::uint32_t>(std::size(gamemode_names));
			}
		}

		if (mode < std::size(gamemode_names) && player.edition == structures::edition_bedrock)
		{
			player.gamemode = static_cast<std::uint8_t>(mode);
			bedrock_player.set_gamemode(*static_cast<structures::session_s*>(player.link), player.gamemode);
			chat.notify(player, std::string{ "gamemode " }.append(gamemode_names[mode]));
		}

		else if (mode < std::size(gamemode_names))
		{
			player.gamemode = static_cast<std::uint8_t>(mode);
			java_player.set_gamemode(*static_cast<structures::client_s*>(player.link), player.gamemode);
			chat.notify(player, std::string{ "gamemode " }.append(gamemode_names[mode]));
		}

		else
		{
			chat.notify(player, "use /gamemode survival, creative, adventure or spectator");
		}
	}
}

//=====================================================================================
