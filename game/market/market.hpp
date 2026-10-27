
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class market_c
	{
	public:

		void load();
		void sell(structures::player_s& player, std::string_view argument);
		void buy(structures::player_s& player, std::uint32_t id);
		std::vector<const structures::listing_s*> filter(std::string_view text) const;
		const structures::listing_s* find(std::uint32_t id) const;

	private:

		void save() const;
		static bool has_room(const structures::player_s& player, const structures::item_stack_s& stack);

		std::vector<structures::listing_s> listings;
		std::uint32_t next_id;
	};

	extern market_c market;
}

//=====================================================================================
