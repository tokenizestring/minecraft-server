
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	hub_c hub;

	void hub_c::load()
	{
		auto path{ functions::get_local_path(menu_file_name) };
		auto file{ std::fopen(path.c_str(), "rb") };
		file = file ? file : create_default(path);
		current = hub_menu_count;

		if (file)
		{
			std::array<char, 2048u> line{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				auto text{ trim(line.data()) };

				if (text.starts_with("menu="))
				{
					add_menu(text.substr(5u));
				}

				else if (text.starts_with("item="))
				{
					add_item(text.substr(5u));
				}

				else if (text.starts_with("hotbar="))
				{
					auto parts{ split(text.substr(7u), '|') };
					auto slot{ hotbar_size };

					if (parts.size() >= 5u)
					{
						std::from_chars(parts[0].data(), parts[0].data() + parts[0].size(), slot);
					}

					if (slot < hotbar_size)
					{
						auto& item{ hotbar[slot] };
						item = { slot, std::string{ parts[1] }, std::string{ parts[2] }, {}, std::string{ parts[4] } };

						for (auto lore : split(parts[3], ';'))
						{
							if (lore.size())
							{
								item.lore.emplace_back(lore);
							}
						}
					}
				}
			}

			std::fclose(file);
		}

		utils::log.write(color_play, "[hub](load) %zu hotbar items, %zu games and %zu info entries", static_cast<std::size_t>(std::count_if(hotbar.begin(), hotbar.end(), [](const auto& item) { return item.item.size() != 0u; })), menus[hub_menu_games].items.size(), menus[hub_menu_info].items.size());
	}
	/*
	//=====================================================================================
	*/
	void hub_c::open(structures::player_s& player, std::uint32_t menu)
	{
		player.menu = menu + 1u;

		if (player.edition == structures::edition_bedrock)
		{
			bedrock_menu.open(*static_cast<structures::session_s*>(player.link), menu);
		}

		else
		{
			java_menu.open(*static_cast<structures::client_s*>(player.link), menu);
		}
	}
	/*
	//=====================================================================================
	*/
	void hub_c::use(structures::player_s& player)
	{
		if (player.held_slot < hotbar_size && hotbar[player.held_slot].item.size())
		{
			run(player, hotbar[player.held_slot].action);
		}
	}
	/*
	//=====================================================================================
	*/
	void hub_c::click(structures::player_s& player, std::uint32_t slot)
	{
		if (player.menu && player.menu <= hub_menu_count)
		{
			for (const auto& item : menus[player.menu - 1u].items)
			{
				if (item.slot == slot)
				{
					run(player, item.action);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void hub_c::choose(structures::player_s& player, std::uint32_t button)
	{
		if (player.menu && player.menu <= hub_menu_count && button < menus[player.menu - 1u].items.size())
		{
			run(player, menus[player.menu - 1u].items[button].action);
		}
	}
	/*
	//=====================================================================================
	*/
	const std::array<structures::menu_s, hub_menu_count>& hub_c::get_menus() const
	{
		return menus;
	}
	/*
	//=====================================================================================
	*/
	const structures::menu_item_s* hub_c::get_hotbar(std::uint32_t slot) const
	{
		return slot < hotbar_size && hotbar[slot].item.size() ? &hotbar[slot] : nullptr;
	}
	/*
	//=====================================================================================
	*/
	void hub_c::add_menu(std::string_view line)
	{
		auto parts{ split(line, '|') };
		current = parts.size() >= 3u && parts[0] == games_command ? hub_menu_games : (parts.size() >= 3u && parts[0] == info_command ? hub_menu_info : hub_menu_count);

		if (current < hub_menu_count)
		{
			auto& menu{ menus[current] };
			menu.title.assign(parts[1]);
			menu.rows = 3u;
			std::from_chars(parts[2].data(), parts[2].data() + parts[2].size(), menu.rows);
			menu.rows = std::clamp(menu.rows, 1u, max_menu_rows);
		}
	}
	/*
	//=====================================================================================
	*/
	void hub_c::add_item(std::string_view line)
	{
		auto parts{ split(line, '|') };
		auto slot{ max_menu_rows * 9u };

		if (parts.size() >= 5u && current < hub_menu_count)
		{
			std::from_chars(parts[0].data(), parts[0].data() + parts[0].size(), slot);
		}

		if (current < hub_menu_count && slot < menus[current].rows * 9u)
		{
			auto& item{ menus[current].items.emplace_back() };
			item.slot = slot;
			item.item.assign(parts[1]);
			item.name.assign(parts[2]);
			item.action.assign(parts[4]);

			for (auto lore : split(parts[3], ';'))
			{
				if (lore.size())
				{
					item.lore.emplace_back(lore);
				}
			}

			std::sort(menus[current].items.begin(), menus[current].items.end(), [](const auto& a, const auto& b) { return a.slot < b.slot; });
		}
	}
	/*
	//=====================================================================================
	*/
	void hub_c::run(structures::player_s& player, std::string_view action)
	{
		auto words{ split(action, ' ') };
		auto verb{ words.size() ? words[0] : std::string_view{} };
		auto rest{ action.size() > verb.size() ? trim(action.substr(verb.size())) : std::string_view{} };

		utils::log.write(color_play, "[hub](action) '%s' %.*s", player.name.c_str(), static_cast<std::int32_t>(action.size()), action.data());

		if (verb == "message")
		{
			chat.notify(player, rest);
		}

		else if (verb == "menu" && rest == games_command)
		{
			open(player, hub_menu_games);
		}

		else if (verb == "menu" && rest == info_command)
		{
			open(player, hub_menu_info);
		}

		else if (verb == friends_command && player.edition == structures::edition_bedrock)
		{
			bedrock_menu.open_social(*static_cast<structures::session_s*>(player.link));
		}

		else if (verb == friends_command)
		{
			java_social.open(*static_cast<structures::client_s*>(player.link));
		}

		else if (verb == "transfer" && words.size() >= 3u && player.edition == structures::edition_java)
		{
			std::uint32_t port{ config.java_port };
			std::from_chars(words[2].data(), words[2].data() + words[2].size(), port);
			java_menu.transfer(*static_cast<structures::client_s*>(player.link), words[1], port);
		}

		else if (verb == "transfer")
		{
			chat.notify(player, "server switching is coming soon on this version");
		}

		else if (verb == rtp_command)
		{
			command.random_teleport(player);
		}

		else if (verb == "world" && rest == smp_command)
		{
			command.travel(player, structures::world_smp);
		}

		else if (verb == "world" && rest == spawn_command)
		{
			command.travel(player, structures::world_spawn);
		}
	}
	/*
	//=====================================================================================
	*/
	std::FILE* hub_c::create_default(const std::string& path) const
	{
		if (auto file{ std::fopen(path.c_str(), "wb") }; file)
		{
			std::fprintf(file, "# hub hotbar java players hold at spawn, right click an item to run its action\n");
			std::fprintf(file, "# hotbar=<slot 0-8>|<item id>|<name>|<lore line;lore line>|<action>\n");
			std::fprintf(file, "# menu=<games|info>|<title>|<rows 1-6>, the item= lines after it fill that menu\n");
			std::fprintf(file, "# item=<slot>|<item id>|<name>|<lore line;lore line>|<action>\n");
			std::fprintf(file, "# actions: menu games | menu info | friends | rtp | message <text> | transfer <host> <port> | world smp | world spawn\n");
			std::fprintf(file, "# colour codes like \xC2\xA7" "a work in names, titles and lore, bedrock opens these with /games, /friends and /info\n\n");
			std::fprintf(file, "hotbar=0|minecraft:compass|\xC2\xA7" "aGames|\xC2\xA7" "7Right click to pick a game|menu games\n");
			std::fprintf(file, "hotbar=4|minecraft:book|\xC2\xA7" "eFriends|\xC2\xA7" "7Right click to see who is online|friends\n");
			std::fprintf(file, "hotbar=8|minecraft:clock|\xC2\xA7" "bInfo|\xC2\xA7" "7Right click for server info|menu info\n\n");
			std::fprintf(file, "menu=games|\xC2\xA7" "2Games|3\n");
			std::fprintf(file, "item=11|minecraft:grass_block|\xC2\xA7" "aJoin the SMP|\xC2\xA7" "7Survive and build with friends;\xC2\xA7" "eClick to play|world smp\n");
			std::fprintf(file, "item=13|minecraft:compass|\xC2\xA7" "bRandom Teleport|\xC2\xA7" "7Start somewhere fresh;\xC2\xA7" "eClick to go|rtp\n");
			std::fprintf(file, "item=15|minecraft:red_bed|\xC2\xA7" "dHome|\xC2\xA7" "7Back to your bed;\xC2\xA7" "eClick to go home|message homes open with the smp\n\n");
			std::fprintf(file, "menu=info|\xC2\xA7" "6Server Info|3\n");
			std::fprintf(file, "item=11|minecraft:book|\xC2\xA7" "eRules|\xC2\xA7" "7Be nice to everyone;\xC2\xA7" "7No griefing or stealing;\xC2\xA7" "7No cheats or hacked clients|message rules: be nice, no griefing, no cheating\n");
			std::fprintf(file, "item=13|minecraft:name_tag|\xC2\xA7" "9Discord|\xC2\xA7" "7Chat with the community|message discord link coming soon\n");
			std::fprintf(file, "item=15|minecraft:emerald|\xC2\xA7" "aStore|\xC2\xA7" "7Support the server|message store coming soon\n");
			std::fclose(file);
		}

		return std::fopen(path.c_str(), "rb");
	}
	/*
	//=====================================================================================
	*/
	std::vector<std::string_view> hub_c::split(std::string_view text, char separator)
	{
		std::vector<std::string_view> parts{};
		auto start{ 0u };

		while (start <= text.size())
		{
			auto end{ std::min(text.find(separator, start), text.size()) };
			parts.push_back(trim(text.substr(start, end - start)));
			start = static_cast<std::uint32_t>(end) + 1u;
		}

		return parts;
	}
	/*
	//=====================================================================================
	*/
	std::string_view hub_c::trim(std::string_view text)
	{
		auto start{ text.find_first_not_of(" \t\r\n") };
		auto end{ text.find_last_not_of(" \t\r\n") };

		return (start == std::string_view::npos) ? std::string_view{} : text.substr(start, end - start + 1u);
	}
}

//=====================================================================================
