
//=====================================================================================

#include "../../stdafx.hpp"

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

		else if (name == roll_command && config.is_operator(player.name))
		{
			roll(player);
		}

		else if (name == roll_command)
		{
			chat.notify(player, "you are not an operator");
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
	void command_c::roll(structures::player_s& player)
	{
		config.smp_seed = functions::make_seed();
		config.store();
		generator.set_seed(config.smp_seed);
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
