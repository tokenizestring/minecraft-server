
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class mobs_c
	{
	public:

		void tick();
		void attack(structures::player_s& player, std::int32_t entity_id);
		void forget(const structures::client_s& client);
		void clear();
		std::size_t count() const;

	private:

		void build_grid();
		void spawn_near(const structures::player_s& player);
		void spawn(std::uint8_t kind, std::double_t x, std::double_t y, std::double_t z);
		void think(structures::mob_s& mob);
		void walk(structures::mob_s& mob);
		void kill(structures::mob_s& mob);
		void play(const structures::mob_s& mob, std::uint32_t which, bool nearby);
		void erase(std::size_t index);
		void refresh();
		void broadcast(structures::mob_s& mob);
		void aim(structures::mob_s& mob, std::double_t direction_x, std::double_t direction_z, std::int32_t range);
		std::uint32_t count_near(std::int32_t chunk_x, std::int32_t chunk_z) const;
		bool is_near_player(const structures::mob_s& mob) const;
		std::vector<structures::client_s*> get_viewers(const structures::mob_s& mob) const;
		std::uint32_t roll(std::uint32_t range);

		static std::int32_t get_chunk(std::double_t value);

		std::vector<structures::mob_s> herd;
		std::unordered_map<std::uint64_t, std::vector<structures::player_s*>> players;
		std::unordered_map<std::uint64_t, std::uint32_t> census;
		std::unordered_map<std::uint64_t, std::vector<structures::client_s*>> watchers;
		std::vector<structures::player_s*> no_players;
		std::vector<structures::client_s*> no_clients;
		std::vector<structures::mob_s> no_herd;
		std::vector<std::size_t> no_mobs;
		std::uint64_t sequence;
		std::uint64_t random;
		std::uint32_t clock;
	};

	extern mobs_c mobs;
}

//=====================================================================================
