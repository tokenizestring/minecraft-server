
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_glow_c
	{
	public:

		void tick();
		void clear(structures::client_s& client);

	private:

		void update(structures::client_s& client);
		void broadcast(const structures::client_s& owner, std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t level);

		static std::uint8_t get_level(const structures::player_s& player);
		static bool is_open(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z);

		std::vector<std::uint8_t> packet;
	};

	extern java_glow_c java_glow;
}

//=====================================================================================
