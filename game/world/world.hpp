
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class world_c
	{
	public:

		std::int32_t allocate_entity_id();
		void add(structures::player_s& player);
		void remove(structures::player_s& player);
		void moved(structures::player_s& player);
		void tick();
		const std::vector<structures::player_s*>& get_players() const;

	private:

		static void spawn(structures::player_s& viewer, const structures::player_s& subject, bool entity);
		static void despawn(structures::player_s& viewer, const structures::player_s& subject);
		static void move(structures::player_s& viewer, const structures::player_s& subject);

		std::vector<structures::player_s*> players;
		std::vector<structures::player_s*> dirty;
		std::int32_t next_entity_id;
	};

	extern world_c world;
}

//=====================================================================================
