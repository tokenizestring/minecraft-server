
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_inventory_c java_inventory;

	void java_inventory_c::send_all(structures::client_s& client)
	{
		auto modern{ functions::is_modern(client.family) };
		auto count{ modern ? modern_window_slots : v1_8_window_slots };

		packet.clear();
		functions::write_varint(packet, modern ? client.layout.play_out_container_content : v1_8_play_out_window_items);

		if (modern)
		{
			client.inventory_state += 1;
			functions::write_varint(packet, 0);
			functions::write_varint(packet, client.inventory_state);
			functions::write_varint(packet, static_cast<std::int32_t>(count));
		}

		else
		{
			functions::write_u8(packet, 0u);
			functions::write_u16(packet, static_cast<std::uint16_t>(count));
		}

		for (auto window_slot{ 0 }; window_slot < static_cast<std::int32_t>(count); ++window_slot)
		{
			write_slot(packet, client, inventory_c::get_slot(window_slot));
		}

		if (modern)
		{
			write_item(packet, client.family, is_hub(client) ? structures::item_stack_s{} : client.player.cursor);
			network.send(client, packet);
		}

		else
		{
			network.send(client, packet);
			packet.clear();
			functions::write_varint(packet, v1_8_play_out_set_slot);
			functions::write_u8(packet, 0xFFu);
			functions::write_u16(packet, 0xFFFFu);
			write_item(packet, client.family, is_hub(client) ? structures::item_stack_s{} : client.player.cursor);
			network.send(client, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::send_slot(structures::client_s& client, std::uint32_t slot)
	{
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_player_inventory);
			functions::write_varint(packet, slot == offhand_slot ? player_inventory_offhand : static_cast<std::int32_t>(slot));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_set_slot);
			functions::write_u8(packet, 0u);
			functions::write_u16(packet, static_cast<std::uint16_t>(inventory_c::get_window_slot(slot)));
		}

		write_slot(packet, client, static_cast<std::int32_t>(slot));
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::handle_click(structures::client_s& client, structures::reader_s& in)
	{
		auto window{ functions::read_varint(in) };
		functions::read_varint(in);
		auto slot{ static_cast<std::int16_t>(functions::read_u16(in)) };
		auto button{ static_cast<std::int8_t>(functions::read_u8(in)) };
		auto mode{ functions::read_varint(in) };
		auto changed{ functions::read_varint(in) };
		std::vector<std::pair<std::int32_t, structures::item_stack_s>> claimed{};

		for (auto index{ 0 }; index < changed && index < static_cast<std::int32_t>(modern_window_slots) && in.good(); ++index)
		{
			auto changed_slot{ static_cast<std::int16_t>(functions::read_u16(in)) };
			claimed.emplace_back(changed_slot, read_hashed(client.family, in));
		}

		auto carried{ read_hashed(client.family, in) };

		if (window == 0 && is_hub(client))
		{
			send_all(client);
		}

		else if (in.good() && window == 0)
		{
			structures::item_stack_s thrown{};
			auto slot_index{ inventory_c::get_slot(slot) };
			auto handled{ (slot_index != -1 || mode == click_drag) && inventory.click(client.player, slot_index, button, mode, client.drag, client.drag_button, thrown) };
			auto kept{ thrown.count && drops.toss(client.player, thrown) == false };
			auto differs{ !handled || kept || !same(carried, client.player.cursor) };

			if (kept)
			{
				inventory.add(client.player, thrown.item, thrown.count);
			}

			for (const auto& [window_slot, stack] : claimed)
			{
				auto index{ inventory_c::get_slot(window_slot) };
				differs = differs || (index >= 0 ? !same(stack, client.player.inventory[static_cast<std::size_t>(index)]) : stack.count != 0u);
			}

			if (differs)
			{
				send_all(client);
			}
		}

		else if (in.good())
		{
			click_window(client, window, slot, button, mode);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::handle_click_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto window{ static_cast<std::int32_t>(functions::read_u8(in)) };
		auto slot{ static_cast<std::int16_t>(functions::read_u16(in)) };
		auto button{ static_cast<std::int32_t>(functions::read_u8(in)) };
		auto action{ functions::read_u16(in) };
		auto mode{ static_cast<std::int32_t>(functions::read_u8(in)) };

		if (in.good())
		{
			packet.clear();
			functions::write_varint(packet, v1_8_play_out_confirm_transaction);
			functions::write_u8(packet, static_cast<std::uint8_t>(window));
			functions::write_u16(packet, action);
			functions::write_bool(packet, window == 0);
			network.send(client, packet);
		}

		if (window == 0 && is_hub(client))
		{
			send_all(client);
		}

		else if (in.good() && window == 0)
		{
			structures::item_stack_s thrown{};
			auto slot_index{ inventory_c::get_slot(slot) };

			if (slot_index != -1 || mode == click_drag)
			{
				inventory.click(client.player, slot_index, button, mode, client.drag, client.drag_button, thrown);
			}

			if (thrown.count && drops.toss(client.player, thrown) == false)
			{
				inventory.add(client.player, thrown.item, thrown.count);
			}

			send_all(client);
		}

		else if (in.good())
		{
			click_window(client, window, slot, button, mode);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::handle_creative(structures::client_s& client, structures::reader_s& in)
	{
		auto slot{ inventory_c::get_slot(static_cast<std::int16_t>(functions::read_u16(in))) };
		auto count{ functions::read_varint(in) };
		auto id{ count > 0 ? functions::read_varint(in) : 0 };

		if (in.good() && slot >= 0 && client.player.gamemode == gamemode_creative && is_hub(client) == false)
		{
			auto item{ items.find_java(client.family, id) };
			client.player.inventory[static_cast<std::size_t>(slot)] = { item, static_cast<std::uint8_t>(item && count > 0 ? std::min(count, 255) : 0) };
		}

		else if (is_hub(client))
		{
			send_all(client);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::handle_creative_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto slot{ inventory_c::get_slot(static_cast<std::int16_t>(functions::read_u16(in))) };
		auto id{ static_cast<std::int16_t>(functions::read_u16(in)) };
		auto count{ id >= 0 ? functions::read_u8(in) : 0u };
		auto damage{ id >= 0 ? functions::read_u16(in) : 0u };

		if (in.good() && slot >= 0 && client.player.gamemode == gamemode_creative && is_hub(client) == false)
		{
			auto item{ id >= 0 ? items.find_java(structures::family_1_8, (static_cast<std::int32_t>(id) << 16) | damage) : std::uint16_t{ 0u } };
			client.player.inventory[static_cast<std::size_t>(slot)] = { item, static_cast<std::uint8_t>(item ? count : 0u) };
		}

		else if (is_hub(client))
		{
			send_all(client);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::handle_held(structures::client_s& client, structures::reader_s& in)
	{
		auto slot{ functions::read_u16(in) };

		if (in.good() && slot < hotbar_size)
		{
			client.player.held_slot = slot;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::swap_hands(structures::client_s& client)
	{
		if (is_hub(client))
		{
			send_all(client);
		}

		else if (functions::is_modern(client.family))
		{
			std::swap(client.player.inventory[client.player.held_slot], client.player.inventory[offhand_slot]);
			send_slot(client, client.player.held_slot);
			send_slot(client, offhand_slot);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::drop(structures::client_s& client, bool all)
	{
		auto& stack{ client.player.inventory[client.player.held_slot] };
		structures::item_stack_s thrown{ stack.item, static_cast<std::uint8_t>(all || stack.count == 0u ? stack.count : 1u) };

		if (is_hub(client))
		{
			send_all(client);
		}

		else if (drops.toss(client.player, thrown))
		{
			stack.count = static_cast<std::uint8_t>(stack.count - thrown.count);
			stack.item = stack.count ? stack.item : 0u;
			send_slot(client, client.player.held_slot);
		}

		else
		{
			send_slot(client, client.player.held_slot);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::write_item(std::vector<std::uint8_t>& out, structures::family_e family, const structures::item_stack_s& stack)
	{
		auto present{ stack.count && stack.item };
		auto id{ present ? items.get_java(family, stack.item) : 0 };

		if (functions::is_modern(family) && present)
		{
			functions::write_varint(out, stack.count);
			functions::write_varint(out, id);
			functions::write_varint(out, 0);
			functions::write_varint(out, 0);
		}

		else if (functions::is_modern(family))
		{
			functions::write_varint(out, 0);
		}

		else if (present)
		{
			functions::write_u16(out, static_cast<std::uint16_t>(id >> 16));
			functions::write_u8(out, stack.count);
			functions::write_u16(out, static_cast<std::uint16_t>(id & 0xFFFF));
			functions::write_u8(out, 0u);
		}

		else
		{
			functions::write_u16(out, 0xFFFFu);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::click_window(structures::client_s& client, std::int32_t window, std::int32_t slot, std::int32_t button, std::int32_t mode)
	{
		if (window == static_cast<std::int32_t>(menu_window_id) && client.player.menu)
		{
			java_menu.click(client, slot);
		}

		else if (window == static_cast<std::int32_t>(market_window_id) && client.market_open)
		{
			java_market.click(client, slot, button, mode);
		}

		else if (window == static_cast<std::int32_t>(social_window_id) && client.social_open)
		{
			java_social.click(client, slot, button, mode);
		}

		send_all(client);
	}
	/*
	//=====================================================================================
	*/
	structures::item_stack_s java_inventory_c::read_hashed(structures::family_e family, structures::reader_s& in)
	{
		structures::item_stack_s stack{};

		if (functions::read_u8(in))
		{
			auto id{ functions::read_varint(in) };
			auto count{ functions::read_varint(in) };
			auto added{ functions::read_varint(in) };

			for (auto index{ 0 }; index < added && in.good(); ++index)
			{
				functions::read_varint(in);
				functions::read_u32(in);
			}

			auto removed{ functions::read_varint(in) };

			for (auto index{ 0 }; index < removed && in.good(); ++index)
			{
				functions::read_varint(in);
			}

			stack = { items.find_java(family, id), static_cast<std::uint8_t>(std::clamp(count, 0, 255)) };
		}

		return stack;
	}
	/*
	//=====================================================================================
	*/
	void java_inventory_c::write_slot(std::vector<std::uint8_t>& out, const structures::client_s& client, std::int32_t slot)
	{
		const auto* hub_item{ is_hub(client) && slot >= 0 ? hub.get_hotbar(static_cast<std::uint32_t>(slot)) : nullptr };

		if (hub_item)
		{
			java_market_c::write_entry(out, client.family, { items.find_name(hub_item->item), 1u }, hub_item->name, hub_item->lore);
		}

		else if (is_hub(client) == false && slot >= 0)
		{
			write_item(out, client.family, client.player.inventory[static_cast<std::size_t>(slot)]);
		}

		else
		{
			write_item(out, client.family, {});
		}
	}
	/*
	//=====================================================================================
	*/
	bool java_inventory_c::is_hub(const structures::client_s& client)
	{
		return client.player.world == structures::world_spawn;
	}
	/*
	//=====================================================================================
	*/
	bool java_inventory_c::same(const structures::item_stack_s& a, const structures::item_stack_s& b)
	{
		auto empty_a{ a.count == 0u || a.item == 0u };
		auto empty_b{ b.count == 0u || b.item == 0u };
		return (empty_a && empty_b) || (a.item == b.item && a.count == b.count);
	}
}

//=====================================================================================
