
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_menu_c java_menu;

	void java_menu_c::build()
	{
		items[structures::family_26_2] = load_items(functions::get_layout(structures::family_26_2).item_path);
		items[structures::family_26_3] = load_items(functions::get_layout(structures::family_26_3).item_path);
		items[structures::family_1_8] = load_items(v1_8_item_path);

		for (auto family : { structures::family_26_2, structures::family_26_3, structures::family_1_8 })
		{
			for (const auto& entry : npc.get_npcs())
			{
				screens[family].push_back(render_screen(family, entry));
				contents[family].push_back(render_content(family, entry));
			}
		}

		utils::log.write(color_play, "[npc](build) %zu java menus, %zu / %zu / %zu items known for 26.2 / 26.3 / 1.8", npc.get_npcs().size(), items[structures::family_26_2].size(), items[structures::family_26_3].size(), items[structures::family_1_8].size());
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::open(structures::client_s& client, std::uint32_t index)
	{
		network.send_held(client, screens[client.family][index]);
		network.send_held(client, contents[client.family][index]);
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::handle_interact(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		auto target{ functions::read_varint(in) };
		auto hand{ id == client.layout.play_in_interact ? functions::read_varint(in) : 0 };

		if (in.good() && hand == 0)
		{
			npc.interact(client.player, target);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::handle_click(structures::client_s& client, structures::reader_s& in)
	{
		auto window{ functions::read_varint(in) };
		functions::read_varint(in);
		auto slot{ static_cast<std::int16_t>(functions::read_u16(in)) };

		if (in.good() && window == static_cast<std::int32_t>(menu_window_id) && client.player.menu)
		{
			finish_click(client, slot);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::handle_close(structures::client_s& client)
	{
		client.player.menu = 0u;
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::handle_use_entity_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto target{ functions::read_varint(in) };
		auto type{ functions::read_varint(in) };

		if (in.good() && type < 2)
		{
			npc.interact(client.player, target);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::handle_click_1_8(structures::client_s& client, structures::reader_s& in)
	{
		auto window{ functions::read_u8(in) };
		auto slot{ static_cast<std::int16_t>(functions::read_u16(in)) };
		functions::read_u8(in);
		auto action{ functions::read_u16(in) };

		if (in.good() && window == menu_window_id && client.player.menu)
		{
			packet.clear();
			functions::write_varint(packet, v1_8_play_out_confirm_transaction);
			functions::write_u8(packet, static_cast<std::uint8_t>(window));
			functions::write_u16(packet, action);
			functions::write_bool(packet, false);
			network.send(client, packet);

			finish_click(client, slot);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::transfer(structures::client_s& client, std::string_view host, std::uint32_t port)
	{
		if (functions::is_modern(client.family))
		{
			packet.clear();
			functions::write_varint(packet, client.layout.play_out_transfer);
			functions::write_string(packet, host);
			functions::write_varint(packet, static_cast<std::int32_t>(port));
			network.send(client, packet);
		}

		else
		{
			chat.notify(client.player, "server switching needs 1.20.5 or newer");
		}
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::finish_click(structures::client_s& client, std::int32_t slot)
	{
		const auto& entry{ npc.get_npcs()[client.player.menu - 1u] };
		auto chosen{ std::any_of(entry.items.begin(), entry.items.end(), [slot](const auto& item) { return static_cast<std::int32_t>(item.slot) == slot; }) };

		network.send_held(client, contents[client.family][client.player.menu - 1u]);
		packet.clear();

		if (functions::is_modern(client.family))
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

		if (chosen)
		{
			packet.clear();
			functions::write_varint(packet, functions::is_modern(client.family) ? client.layout.play_out_container_close : v1_8_play_out_close_window);

			if (functions::is_modern(client.family))
			{
				functions::write_varint(packet, static_cast<std::int32_t>(menu_window_id));
			}

			else
			{
				functions::write_u8(packet, static_cast<std::uint8_t>(menu_window_id));
			}

			network.send(client, packet);
			npc.click(client.player, static_cast<std::uint32_t>(slot));
			client.player.menu = 0u;
		}
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<std::uint8_t>> java_menu_c::render_screen(structures::family_e family, const structures::npc_s& entry)
	{
		auto out{ std::make_shared<std::vector<std::uint8_t>>() };
		packet.clear();

		if (functions::is_modern(family))
		{
			functions::write_varint(packet, functions::get_layout(family).play_out_open_screen);
			functions::write_varint(packet, static_cast<std::int32_t>(menu_window_id));
			functions::write_varint(packet, static_cast<std::int32_t>(entry.rows - 1u));
			functions::write_nbt_text(packet, entry.title);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_open_window);
			functions::write_u8(packet, static_cast<std::uint8_t>(menu_window_id));
			functions::write_string(packet, v1_8_chest_type);
			functions::write_json_text(packet, entry.title);
			functions::write_u8(packet, static_cast<std::uint8_t>(entry.rows * 9u));
		}

		functions::write_frame(*out, packet);
		return out;
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<std::uint8_t>> java_menu_c::render_content(structures::family_e family, const structures::npc_s& entry)
	{
		auto out{ std::make_shared<std::vector<std::uint8_t>>() };
		packet.clear();

		if (functions::is_modern(family))
		{
			functions::write_varint(packet, functions::get_layout(family).play_out_container_content);
			functions::write_varint(packet, static_cast<std::int32_t>(menu_window_id));
			functions::write_varint(packet, 0);
			functions::write_varint(packet, static_cast<std::int32_t>(entry.rows * 9u));
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_window_items);
			functions::write_u8(packet, static_cast<std::uint8_t>(menu_window_id));
			functions::write_u16(packet, static_cast<std::uint16_t>(entry.rows * 9u));
		}

		for (auto slot{ 0u }; slot < entry.rows * 9u; ++slot)
		{
			auto item{ std::find_if(entry.items.begin(), entry.items.end(), [slot](const auto& candidate) { return candidate.slot == slot; }) };

			if (item != entry.items.end() && functions::is_modern(family))
			{
				write_item_modern(packet, family, *item);
			}

			else if (item != entry.items.end())
			{
				write_item_1_8(packet, *item);
			}

			else if (functions::is_modern(family))
			{
				functions::write_varint(packet, 0);
			}

			else
			{
				functions::write_u16(packet, 0xFFFFu);
			}
		}

		if (functions::is_modern(family))
		{
			functions::write_varint(packet, 0);
		}

		functions::write_frame(*out, packet);
		return out;
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::write_item_modern(std::vector<std::uint8_t>& out, structures::family_e family, const structures::menu_item_s& item) const
	{
		auto found{ items[family].find(item.item) };

		functions::write_varint(out, 1);
		functions::write_varint(out, found != items[family].end() ? found->second : modern_item_fallback);
		functions::write_varint(out, item.lore.size() ? 2 : 1);
		functions::write_varint(out, 0);
		functions::write_varint(out, modern_component_item_name);
		functions::write_nbt_plain(out, item.name);

		if (item.lore.size())
		{
			functions::write_varint(out, modern_component_lore);
			functions::write_varint(out, static_cast<std::int32_t>(item.lore.size()));

			for (const auto& line : item.lore)
			{
				functions::write_nbt_plain(out, line);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void java_menu_c::write_item_1_8(std::vector<std::uint8_t>& out, const structures::menu_item_s& item) const
	{
		auto found{ items[structures::family_1_8].find(item.item) };
		auto packed{ found != items[structures::family_1_8].end() ? found->second : v1_8_item_fallback };

		functions::write_u16(out, static_cast<std::uint16_t>(packed >> 16));
		functions::write_u8(out, 1u);
		functions::write_u16(out, static_cast<std::uint16_t>(packed & 0xFFFF));
		functions::write_u8(out, 0x0Au);
		functions::write_u16(out, 0u);
		functions::write_u8(out, 0x0Au);
		functions::write_nbt_string(out, "display");
		functions::write_u8(out, 0x08u);
		functions::write_nbt_string(out, "Name");
		functions::write_nbt_string(out, std::string{ v1_8_reset }.append(item.name));

		if (item.lore.size())
		{
			functions::write_u8(out, 0x09u);
			functions::write_nbt_string(out, "Lore");
			functions::write_u8(out, 0x08u);
			functions::write_i32(out, static_cast<std::int32_t>(item.lore.size()));

			for (const auto& line : item.lore)
			{
				functions::write_nbt_string(out, std::string{ v1_8_reset }.append(line));
			}
		}

		functions::write_u8(out, 0u);
		functions::write_u8(out, 0u);
	}
	/*
	//=====================================================================================
	*/
	std::unordered_map<std::string, std::int32_t> java_menu_c::load_items(const char* path)
	{
		std::unordered_map<std::string, std::int32_t> table{};

		if (auto file{ std::fopen(functions::get_local_path(path).c_str(), "rb") }; file)
		{
			std::array<char, 256u> line{};
			std::array<char, 128u> name{};
			auto index{ 0 };

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::int32_t id{};
				std::int32_t damage{};

				if (std::sscanf(line.data(), "%127s %d %d", name.data(), &id, &damage) == 3)
				{
					table[name.data()] = (id << 16) | (damage & 0xFFFF);
				}

				else if (std::sscanf(line.data(), "%127s", name.data()) == 1)
				{
					table[name.data()] = index;
				}

				index += 1;
			}

			std::fclose(file);
		}

		return table;
	}
}

//=====================================================================================
