
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_social_c java_social;

	void java_social_c::open(structures::client_s& client)
	{
		client.social_open = true;
		client.social_page = 0u;
		client.market_open = false;
		client.player.menu = 0u;
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_open_screen);
			functions::write_varint(packet, static_cast<std::int32_t>(social_window_id));
			functions::write_varint(packet, static_cast<std::int32_t>(market_rows - 1u));
			functions::write_nbt_text(packet, social_title);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_open_window);
			functions::write_u8(packet, static_cast<std::uint8_t>(social_window_id));
			functions::write_string(packet, v1_8_chest_type);
			functions::write_json_text(packet, social_title);
			functions::write_u8(packet, static_cast<std::uint8_t>(market_rows * 9u));
		}

		network.send(client, packet);
		render(client);
	}
	/*
	//=====================================================================================
	*/
	void java_social_c::click(structures::client_s& client, std::int32_t slot, std::int32_t button, std::int32_t mode)
	{
		auto picked{ mode == click_pickup && (button == 0 || button == 1) && slot >= 0 };

		if (picked && slot < static_cast<std::int32_t>(client.social_view.size()) && client.social_view[static_cast<std::size_t>(slot)].size())
		{
			friends.click(client.player, client.social_view[static_cast<std::size_t>(slot)], button == 1);
		}

		else if (picked && slot == static_cast<std::int32_t>(market_prev_slot) && client.social_page)
		{
			client.social_page -= 1u;
		}

		else if (picked && slot == static_cast<std::int32_t>(market_next_slot))
		{
			client.social_page += 1u;
		}

		render(client);
	}
	/*
	//=====================================================================================
	*/
	void java_social_c::render(structures::client_s& client)
	{
		static constexpr const char* colours[] = { "\xC2\xA7" "e", "\xC2\xA7" "a", "\xC2\xA7" "f", "\xC2\xA7" "f", "\xC2\xA7" "7" };
		static constexpr const char* hints[] = { "click to accept", "right click to remove", "waiting for them", "click to add friend", "right click to remove" };

		auto entries{ friends.list(client.player) };
		auto pages{ std::max<std::uint32_t>(1u, static_cast<std::uint32_t>((entries.size() + market_page_size - 1u) / market_page_size)) };
		auto modern{ functions::is_modern(client.family) };
		auto head{ items.find_name("minecraft:player_head") };
		auto online{ std::count_if(entries.begin(), entries.end(), [](const auto& entry) { return entry.state == social_friend_online; }) };
		auto total{ client.player.friends.size() };
		client.social_page = std::min(client.social_page, pages - 1u);
		client.social_view.assign(market_page_size, std::string{});
		packet.clear();

		if (modern)
		{
			functions::write_varint(packet, client.layout.play_out_container_content);
			functions::write_varint(packet, static_cast<std::int32_t>(social_window_id));
			functions::write_varint(packet, 0);
			functions::write_varint(packet, static_cast<std::int32_t>(market_rows * 9u));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_window_items);
			functions::write_u8(packet, static_cast<std::uint8_t>(social_window_id));
			functions::write_u16(packet, static_cast<std::uint16_t>(market_rows * 9u));
		}

		for (auto slot{ 0u }; slot < market_rows * 9u; ++slot)
		{
			auto index{ static_cast<std::size_t>(client.social_page) * market_page_size + slot };

			if (slot < market_page_size && index < entries.size())
			{
				const auto& entry{ entries[index] };
				client.social_view[slot] = entry.name;
				java_market_c::write_entry(packet, client.family, { head, 1u }, std::string{ colours[entry.state] }.append(entry.name), { social_labels[entry.state], hints[entry.state] });
			}

			else if (slot == market_prev_slot && client.social_page)
			{
				java_market_c::write_entry(packet, client.family, { items.find_name("arrow"), 1u }, "previous page", {});
			}

			else if (slot == market_next_slot && client.social_page + 1u < pages)
			{
				java_market_c::write_entry(packet, client.family, { items.find_name("arrow"), 1u }, "next page", {});
			}

			else if (slot == market_info_slot)
			{
				java_market_c::write_entry(packet, client.family, { items.find_name("book"), 1u }, std::to_string(total) + " friends, " + std::to_string(online) + " online", { "page " + std::to_string(client.social_page + 1u) + " of " + std::to_string(pages), "click a player to add or accept", "right click a friend to remove" });
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
	}
}

//=====================================================================================
