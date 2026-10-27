
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class inventory_c
	{
	public:

		std::uint32_t add(structures::player_s& player, std::uint16_t item, std::uint32_t count) const;
		bool click(structures::player_s& player, std::int32_t slot, std::int32_t button, std::int32_t mode, std::vector<std::int32_t>& drag, std::int32_t& drag_button) const;

		static std::int32_t get_slot(std::int32_t window_slot);
		static std::int32_t get_window_slot(std::uint32_t slot);

	private:

		bool pickup(structures::player_s& player, std::int32_t slot, std::int32_t button) const;
		void quick_move(structures::player_s& player, std::int32_t slot) const;
		bool finish_drag(structures::player_s& player, const std::vector<std::int32_t>& drag, std::int32_t button) const;
		void collect(structures::player_s& player) const;
		std::uint32_t merge(structures::player_s& player, structures::item_stack_s& stack, std::uint32_t first, std::uint32_t last) const;
	};

	extern inventory_c inventory;
}

//=====================================================================================
