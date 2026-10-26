
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
	};

	extern command_c command;
}

//=====================================================================================
