
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

		if (name == "chat")
		{
			chat.toggle(player, argument);
		}

		else
		{
			chat.notify(player, "unknown command");
		}
	}
}

//=====================================================================================
