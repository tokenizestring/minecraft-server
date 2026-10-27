
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	inventory_c inventory;

	std::uint32_t inventory_c::add(structures::player_s& player, std::uint16_t item, std::uint32_t count) const
	{
		structures::item_stack_s stack{ item, static_cast<std::uint8_t>(std::min<std::uint32_t>(count, 255u)) };
		return merge(player, stack, 0u, storage_size);
	}
	/*
	//=====================================================================================
	*/
	bool inventory_c::click(structures::player_s& player, std::int32_t slot, std::int32_t button, std::int32_t mode, std::vector<std::int32_t>& drag, std::int32_t& drag_button, structures::item_stack_s& thrown) const
	{
		auto handled{ true };
		auto creative{ player.gamemode == gamemode_creative };
		auto inside{ slot >= 0 && slot < static_cast<std::int32_t>(inventory_size) };
		auto& cursor{ player.cursor };
		thrown = {};

		if (mode == click_pickup)
		{
			handled = (inside || slot == click_outside) && pickup(player, slot, button, thrown);
		}

		else if (mode == click_quick_move && inside)
		{
			quick_move(player, slot);
		}

		else if (mode == click_swap && inside && button >= 0 && button < static_cast<std::int32_t>(hotbar_size))
		{
			std::swap(player.inventory[static_cast<std::size_t>(slot)], player.inventory[static_cast<std::size_t>(button)]);
		}

		else if (mode == click_clone && inside && creative && cursor.count == 0u && player.inventory[static_cast<std::size_t>(slot)].count)
		{
			cursor = player.inventory[static_cast<std::size_t>(slot)];
			cursor.count = items.get_stack(cursor.item);
		}

		else if (mode == click_throw && inside)
		{
			auto& stack{ player.inventory[static_cast<std::size_t>(slot)] };
			thrown = { stack.item, static_cast<std::uint8_t>(button == 0 && stack.count ? 1u : stack.count) };
			stack.count = static_cast<std::uint8_t>(stack.count - thrown.count);
			stack.item = stack.count ? stack.item : 0u;
		}

		else if (mode == click_drag && button % 4 == 0)
		{
			drag.clear();
			drag_button = button;
		}

		else if (mode == click_drag && button % 4 == 1 && inside)
		{
			drag.push_back(slot);
		}

		else if (mode == click_drag && button % 4 == 2)
		{
			handled = finish_drag(player, drag, button);
			drag.clear();
		}

		else if (mode == click_collect && cursor.count)
		{
			collect(player);
		}

		else
		{
			handled = mode == click_clone || mode == click_collect || (mode == click_throw && slot == click_outside);
		}

		return handled;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t inventory_c::get_slot(std::int32_t window_slot)
	{
		auto slot{ -1 };

		if (window_slot >= window_main_first && window_slot <= window_main_last)
		{
			slot = window_slot;
		}

		else if (window_slot >= hotbar_first_slot && window_slot <= window_hotbar_last)
		{
			slot = window_slot - hotbar_first_slot;
		}

		else if (window_slot == window_offhand_slot)
		{
			slot = static_cast<std::int32_t>(offhand_slot);
		}

		else if (window_slot == click_outside)
		{
			slot = click_outside;
		}

		return slot;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t inventory_c::get_window_slot(std::uint32_t slot)
	{
		return slot < hotbar_size ? static_cast<std::int32_t>(slot) + hotbar_first_slot : (slot == offhand_slot ? window_offhand_slot : static_cast<std::int32_t>(slot));
	}
	/*
	//=====================================================================================
	*/
	bool inventory_c::pickup(structures::player_s& player, std::int32_t slot, std::int32_t button, structures::item_stack_s& thrown) const
	{
		auto& cursor{ player.cursor };
		auto handled{ button == 0 || button == 1 };

		if (slot == click_outside && handled)
		{
			thrown = { cursor.item, static_cast<std::uint8_t>(button == 0 || cursor.count == 0u ? cursor.count : 1u) };
			cursor.count = static_cast<std::uint8_t>(cursor.count - thrown.count);
		}

		else if (handled)
		{
			auto& stack{ player.inventory[static_cast<std::size_t>(slot)] };
			auto same{ stack.count == 0u || stack.item == cursor.item };
			auto room{ static_cast<std::uint32_t>(items.get_stack(cursor.item)) - (stack.count ? stack.count : 0u) };

			if (cursor.count == 0u && button == 0)
			{
				cursor = stack;
				stack = {};
			}

			else if (cursor.count == 0u)
			{
				auto taken{ static_cast<std::uint8_t>((stack.count + 1u) / 2u) };
				cursor = { stack.item, taken };
				stack.count = static_cast<std::uint8_t>(stack.count - taken);
			}

			else if (same && button == 0)
			{
				auto moved{ std::min<std::uint32_t>(cursor.count, room) };
				stack.item = cursor.item;
				stack.count = static_cast<std::uint8_t>(stack.count + moved);
				cursor.count = static_cast<std::uint8_t>(cursor.count - moved);
			}

			else if (same && room)
			{
				stack.item = cursor.item;
				stack.count = static_cast<std::uint8_t>(stack.count + 1u);
				cursor.count = static_cast<std::uint8_t>(cursor.count - 1u);
			}

			else if (stack.count && stack.item != cursor.item)
			{
				std::swap(stack, cursor);
			}

			stack.item = stack.count ? stack.item : 0u;
		}

		cursor.item = cursor.count ? cursor.item : 0u;
		return handled;
	}
	/*
	//=====================================================================================
	*/
	void inventory_c::quick_move(structures::player_s& player, std::int32_t slot) const
	{
		auto& stack{ player.inventory[static_cast<std::size_t>(slot)] };
		auto to_main{ slot < static_cast<std::int32_t>(hotbar_size) || slot == static_cast<std::int32_t>(offhand_slot) };

		if (stack.count)
		{
			stack.count = static_cast<std::uint8_t>(merge(player, stack, to_main ? hotbar_size : 0u, to_main ? storage_size : hotbar_size));
			stack.item = stack.count ? stack.item : 0u;
		}
	}
	/*
	//=====================================================================================
	*/
	bool inventory_c::finish_drag(structures::player_s& player, const std::vector<std::int32_t>& drag, std::int32_t button) const
	{
		auto& cursor{ player.cursor };
		auto kind{ button / 4 };
		auto handled{ drag.size() && cursor.count && (kind == 0 || kind == 1 || (kind == 2 && player.gamemode == gamemode_creative)) };

		if (handled)
		{
			auto share{ kind == 0 ? std::max<std::uint32_t>(1u, cursor.count / static_cast<std::uint32_t>(drag.size())) : kind == 1 ? 1u : items.get_stack(cursor.item) };
			auto maximum{ static_cast<std::uint32_t>(items.get_stack(cursor.item)) };

			for (auto slot : drag)
			{
				auto& stack{ player.inventory[static_cast<std::size_t>(slot)] };
				auto fits{ stack.count == 0u || stack.item == cursor.item };
				auto given{ fits ? std::min({ share, maximum - stack.count, kind == 2 ? share : static_cast<std::uint32_t>(cursor.count) }) : 0u };

				stack.item = given ? cursor.item : stack.item;
				stack.count = static_cast<std::uint8_t>(stack.count + given);
				cursor.count = static_cast<std::uint8_t>(kind == 2 ? cursor.count : cursor.count - given);
			}

			cursor.item = cursor.count ? cursor.item : 0u;
		}

		return handled;
	}
	/*
	//=====================================================================================
	*/
	void inventory_c::collect(structures::player_s& player) const
	{
		auto& cursor{ player.cursor };
		auto maximum{ static_cast<std::uint32_t>(items.get_stack(cursor.item)) };

		for (auto& stack : player.inventory)
		{
			if (stack.count && stack.item == cursor.item && cursor.count < maximum)
			{
				auto taken{ std::min<std::uint32_t>(stack.count, maximum - cursor.count) };
				cursor.count = static_cast<std::uint8_t>(cursor.count + taken);
				stack.count = static_cast<std::uint8_t>(stack.count - taken);
				stack.item = stack.count ? stack.item : 0u;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t inventory_c::merge(structures::player_s& player, structures::item_stack_s& stack, std::uint32_t first, std::uint32_t last) const
	{
		auto left{ static_cast<std::uint32_t>(stack.count) };
		auto maximum{ static_cast<std::uint32_t>(items.get_stack(stack.item)) };

		for (auto pass{ 0u }; pass < 2u; ++pass)
		{
			for (auto slot{ first }; slot < last && left; ++slot)
			{
				auto& target{ player.inventory[slot] };
				auto usable{ pass == 0u ? target.count && target.item == stack.item : target.count == 0u };

				if (usable && &target != &stack)
				{
					auto moved{ std::min(left, maximum - target.count) };
					target.item = stack.item;
					target.count = static_cast<std::uint8_t>(target.count + moved);
					left -= moved;
				}
			}
		}

		return left;
	}
}

//=====================================================================================
