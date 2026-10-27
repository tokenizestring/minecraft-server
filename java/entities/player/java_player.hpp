
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_player_c
	{
	public:

		void spawn(structures::client_s& viewer, const structures::player_s& subject, bool entity);
		void move(structures::client_s& viewer, const structures::player_s& subject);
		void despawn(structures::client_s& viewer, const structures::player_s& subject);
		void set_gamemode(structures::client_s& client, std::uint8_t mode);

	private:

		std::vector<std::uint8_t> packet;
	};

	extern java_player_c java_player;
}

//=====================================================================================
