
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_market_c java_market;

	void java_market_c::open(structures::client_s& client, std::string_view filter)
	{
		auto title{ filter.size() ? std::string{ market_title }.append(": ").append(filter) : std::string{ market_title } };

		client.market_open = true;
		client.market_page = 0u;
		client.market_filter.assign(filter);
		client.player.menu = 0u;
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_open_screen);
			functions::write_varint(packet, static_cast<std::int32_t>(market_window_id));
			functions::write_varint(packet, static_cast<std::int32_t>(market_rows - 1u));
			functions::write_nbt_text(packet, title);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_open_window);
			functions::write_u8(packet, static_cast<std::uint8_t>(market_window_id));
			functions::write_string(packet, v1_8_chest_type);
			functions::write_json_text(packet, title);
			functions::write_u8(packet, static_cast<std::uint8_t>(market_rows * 9u));
		}

		network.send(client, packet);
		render(client);
	}
	/*
	//=====================================================================================
	*/
	void java_market_c::click(structures::client_s& client, std::int32_t slot, std::int32_t button, std::int32_t mode)
	{
		auto picked{ mode == click_pickup && button == 0 && slot >= 0 };

		if (picked && slot < static_cast<std::int32_t>(market_page_size) && client.market_view[static_cast<std::size_t>(slot)])
		{
			market.buy(client.player, client.market_view[static_cast<std::size_t>(slot)]);
		}

		else if (picked && slot == static_cast<std::int32_t>(market_prev_slot) && client.market_page)
		{
			client.market_page -= 1u;
		}

		else if (picked && slot == static_cast<std::int32_t>(market_next_slot))
		{
			client.market_page += 1u;
		}

		render(client);
	}
	/*
	//=====================================================================================
	*/
	void java_market_c::render(structures::client_s& client)
	{
		auto listings{ market.filter(client.market_filter) };
		auto pages{ std::max<std::uint32_t>(1u, static_cast<std::uint32_t>((listings.size() + market_page_size - 1u) / market_page_size)) };
		auto modern{ functions::is_modern(client.family) };
		client.market_page = std::min(client.market_page, pages - 1u);
		client.market_view.fill(0u);
		packet.clear();

		if (modern)
		{
			functions::write_varint(packet, client.layout.play_out_container_content);
			functions::write_varint(packet, static_cast<std::int32_t>(market_window_id));
			functions::write_varint(packet, 0);
			functions::write_varint(packet, static_cast<std::int32_t>(market_rows * 9u));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_window_items);
			functions::write_u8(packet, static_cast<std::uint8_t>(market_window_id));
			functions::write_u16(packet, static_cast<std::uint16_t>(market_rows * 9u));
		}

		for (auto slot{ 0u }; slot < market_rows * 9u; ++slot)
		{
			auto index{ static_cast<std::size_t>(client.market_page) * market_page_size + slot };

			if (slot < market_page_size && index < listings.size())
			{
				const auto& listing{ *listings[index] };
				auto own{ listing.seller == client.player.name };
				client.market_view[slot] = listing.id;
				write_entry(packet, client.family, listing.stack, {}, { "price " + std::to_string(listing.price), "seller " + listing.seller, own ? "your listing, click to take back" : "click to buy" });
			}

			else if (slot == market_prev_slot && client.market_page)
			{
				write_entry(packet, client.family, { items.find_name("arrow"), 1u }, "previous page", {});
			}

			else if (slot == market_next_slot && client.market_page + 1u < pages)
			{
				write_entry(packet, client.family, { items.find_name("arrow"), 1u }, "next page", {});
			}

			else if (slot == market_info_slot)
			{
				write_entry(packet, client.family, { items.find_name("emerald"), 1u }, "balance " + std::to_string(client.player.balance), { "page " + std::to_string(client.market_page + 1u) + " of " + std::to_string(pages), "/sell <price> lists the item in your hand", "/buy <name> searches" });
			}

			else
			{
				java_inventory_c::write_item(packet, client.family, {});
			}
		}

		if (modern)
		{
			functions::write_varint(packet, 0);
		}

		network.send(client, packet);
		packet.clear();

		if (modern)
		{
			functions::write_varint(packet, client.layout.play_out_cursor_item);
			functions::write_varint(packet, 0);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_set_slot);
			functions::write_u8(packet, 0xFFu);
			functions::write_u16(packet, 0xFFFFu);
			functions::write_u16(packet, 0xFFFFu);
		}

		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_market_c::write_entry(std::vector<std::uint8_t>& out, structures::family_e family, const structures::item_stack_s& stack, std::string_view name, const std::vector<std::string>& lore)
	{
		auto id{ items.get_java(family, stack.item) };

		if (functions::is_modern(family))
		{
			functions::write_varint(out, stack.count);
			functions::write_varint(out, id);
			functions::write_varint(out, (name.size() ? 1 : 0) + (lore.size() ? 1 : 0));
			functions::write_varint(out, 0);

			if (name.size())
			{
				functions::write_varint(out, modern_component_item_name);
				functions::write_nbt_plain(out, name);
			}

			if (lore.size())
			{
				functions::write_varint(out, modern_component_lore);
				functions::write_varint(out, static_cast<std::int32_t>(lore.size()));

				for (const auto& line : lore)
				{
					functions::write_nbt_plain(out, line);
				}
			}
		}

		else
		{
			functions::write_u16(out, static_cast<std::uint16_t>(id >> 16));
			functions::write_u8(out, stack.count);
			functions::write_u16(out, static_cast<std::uint16_t>(id & 0xFFFF));
			functions::write_u8(out, 0x0Au);
			functions::write_u16(out, 0u);
			functions::write_u8(out, 0x0Au);
			functions::write_nbt_string(out, "display");

			if (name.size())
			{
				functions::write_u8(out, 0x08u);
				functions::write_nbt_string(out, "Name");
				functions::write_nbt_string(out, std::string{ v1_8_reset }.append(name));
			}

			functions::write_u8(out, 0x09u);
			functions::write_nbt_string(out, "Lore");
			functions::write_u8(out, 0x08u);
			functions::write_i32(out, static_cast<std::int32_t>(lore.size()));

			for (const auto& line : lore)
			{
				functions::write_nbt_string(out, std::string{ v1_8_reset }.append(line));
			}

			functions::write_u8(out, 0u);
			functions::write_u8(out, 0u);
		}
	}
}

//=====================================================================================
