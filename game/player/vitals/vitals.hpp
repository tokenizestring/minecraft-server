
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class vitals_c
	{
	public:

		void tick();
		void reset(structures::player_s& player) const;
		void moved(structures::player_s& player, std::double_t x, std::double_t y, std::double_t z, bool on_ground) const;
		void damage(structures::player_s& player, std::float_t amount) const;
		void exhaust(structures::player_s& player, std::float_t amount) const;
		void start_eating(structures::player_s& player) const;
		void stop_eating(structures::player_s& player) const;

		static bool is_live(const structures::player_s& player);

	private:

		void update(structures::player_s& player) const;
		void finish_eating(structures::player_s& player) const;
		void die(structures::player_s& player) const;
	};

	extern vitals_c vitals;
}

//=====================================================================================
