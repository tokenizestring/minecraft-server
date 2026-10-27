
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class command_c
	{
	public:

		void run(structures::player_s& player, std::string_view line);
		void travel(structures::player_s& player, structures::world_e target);

	private:

		void roll(structures::player_s& player);
		void pay(structures::player_s& player, std::string_view argument);

		static void set_gamemode(structures::player_s& player, std::string_view argument);
	};

	extern command_c command;
}

//=====================================================================================
