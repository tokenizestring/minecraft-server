
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class drops_c
	{
	public:

		void load();
		void save() const;
		bool spawn(structures::world_e target, const structures::item_stack_s& stack, std::double_t x, std::double_t y, std::double_t z);
		bool toss(structures::player_s& thrower, const structures::item_stack_s& stack);
		void clear(structures::world_e target);
		void tick();

	private:

		using grid_t = std::array<std::unordered_map<std::uint64_t, std::vector<structures::player_s*>>, structures::world_count>;

		void build_grid();
		void step(structures::drop_s& drop);
		void pull(structures::drop_s& drop, const structures::player_s& target, std::double_t distance);
		void fall(structures::drop_s& drop);
		bool collect(std::size_t index, structures::player_s& target);
		void erase(std::size_t index);
		void refresh();
		void show(const structures::drop_s& drop);
		void broadcast_move(structures::drop_s& drop);
		structures::player_s* find_target(const structures::drop_s& drop, std::double_t& distance) const;
		structures::player_s* find_player(std::int32_t entity_id) const;
		std::vector<structures::client_s*> get_viewers(const structures::drop_s& drop) const;

		static bool is_solid(structures::world_e target, std::double_t x, std::double_t y, std::double_t z);
		static std::double_t get_floor(structures::world_e target, std::double_t x, std::double_t y, std::double_t z);
		static std::int32_t get_chunk(std::double_t value);

		std::vector<structures::drop_s> ground;
		grid_t nearby;
		std::vector<structures::player_s*> no_players;
		std::vector<std::size_t> no_drops;
		std::uint64_t sequence;
		std::uint32_t clock;
	};

	extern drops_c drops;
}

//=====================================================================================
