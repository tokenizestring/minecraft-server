
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	npc_c npc;

	void npc_c::load()
	{
		auto path{ functions::get_local_path(npc_file_name) };
		auto file{ std::fopen(path.c_str(), "rb") };
		file = file ? file : create_default(path);

		if (file)
		{
			std::array<char, 2048u> line{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				auto text{ trim(line.data()) };

				if (text.starts_with("npc="))
				{
					add_npc(text.substr(4u));
				}

				else if (text.starts_with("item="))
				{
					add_item(text.substr(5u));
				}
			}

			std::fclose(file);
		}

		for (auto& entry : npcs)
		{
			auto& player{ entry.player };
			player.entity_id = world.allocate_entity_id();
			functions::make_offline_uuid(std::string{ "npc:" }.append(player.name), player.uuid_high, player.uuid_low);

			auto block_x{ static_cast<std::int32_t>(std::floor(player.x)) };
			auto block_y{ static_cast<std::int32_t>(std::floor(player.y)) };
			auto block_z{ static_cast<std::int32_t>(std::floor(player.z)) };

			for (auto y{ block_y - npc_ground_search }; y < block_y; ++y)
			{
				if (terrain.is_solid_at(block_x, y, block_z))
				{
					player.y = static_cast<std::double_t>(y + 1);
				}
			}

			utils::log.write(color_play, "[npc](load) '%s' entity %d at %.1f %.1f %.1f, %zu menu items", player.name.c_str(), player.entity_id, player.x, player.y, player.z, entry.items.size());
		}
	}
	/*
	//=====================================================================================
	*/
	void npc_c::show(structures::player_s& viewer)
	{
		viewer.npc_angles.fill(0u);

		for (const auto& entry : npcs)
		{
			if (viewer.edition == structures::edition_bedrock)
			{
				bedrock_player.spawn(*static_cast<structures::session_s*>(viewer.link), entry.player, true);
			}

			else
			{
				java_player.spawn(*static_cast<structures::client_s*>(viewer.link), entry.player, true);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void npc_c::look(structures::player_s& viewer)
	{
		for (auto index{ 0u }; index < npcs.size(); ++index)
		{
			const auto& subject{ npcs[index].player };
			auto x{ viewer.x - subject.x };
			auto y{ viewer.y - subject.y };
			auto z{ viewer.z - subject.z };
			auto in_range{ x * x + y * y + z * z <= npc_look_range * npc_look_range };
			auto yaw{ in_range ? static_cast<std::float_t>(std::atan2(-x, z) * degrees_per_radian) : subject.yaw };
			auto pitch{ in_range ? static_cast<std::float_t>(-std::atan2(y, std::sqrt(x * x + z * z)) * degrees_per_radian) : 0.0f };
			auto angles{ npc_angle_sent | (static_cast<std::uint32_t>(functions::get_angle_byte(yaw)) << 8u) | functions::get_angle_byte(pitch) };

			if (angles != viewer.npc_angles[index] && viewer.edition == structures::edition_bedrock)
			{
				viewer.npc_angles[index] = angles;
				bedrock_player.look(*static_cast<structures::session_s*>(viewer.link), subject, yaw, pitch);
			}

			else if (angles != viewer.npc_angles[index])
			{
				viewer.npc_angles[index] = angles;
				java_player.look(*static_cast<structures::client_s*>(viewer.link), subject, yaw, pitch);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void npc_c::interact(structures::player_s& player, std::int32_t entity_id)
	{
		for (auto index{ 0u }; index < npcs.size(); ++index)
		{
			if (npcs[index].player.entity_id == entity_id && player.edition == structures::edition_bedrock)
			{
				player.menu = index + 1u;
				bedrock_menu.open(*static_cast<structures::session_s*>(player.link), index);
			}

			else if (npcs[index].player.entity_id == entity_id)
			{
				player.menu = index + 1u;
				java_menu.open(*static_cast<structures::client_s*>(player.link), index);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void npc_c::click(structures::player_s& player, std::uint32_t slot)
	{
		if (player.menu && player.menu <= npcs.size())
		{
			for (const auto& item : npcs[player.menu - 1u].items)
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
	void npc_c::choose(structures::player_s& player, std::uint32_t button)
	{
		if (player.menu && player.menu <= npcs.size() && button < npcs[player.menu - 1u].items.size())
		{
			run(player, npcs[player.menu - 1u].items[button].action);
		}
	}
	/*
	//=====================================================================================
	*/
	const std::vector<structures::npc_s>& npc_c::get_npcs() const
	{
		return npcs;
	}
	/*
	//=====================================================================================
	*/
	void npc_c::add_npc(std::string_view line)
	{
		auto parts{ split(line, '|') };

		if (parts.size() >= 4u && npcs.size() < max_npcs)
		{
			auto position{ split(parts[1], ' ') };
			auto& entry{ npcs.emplace_back() };
			std::array<std::double_t, 4u> values{ spawn_x, spawn_y, spawn_z, 0.0 };

			for (auto index{ 0u }; index < std::min<std::size_t>(position.size(), values.size()); ++index)
			{
				std::from_chars(position[index].data(), position[index].data() + position[index].size(), values[index]);
			}

			entry.player.name.assign(parts[0].substr(0u, max_name_length));
			entry.player.x = values[0];
			entry.player.y = values[1];
			entry.player.z = values[2];
			entry.player.yaw = static_cast<std::float_t>(values[3]);
			entry.player.on_ground = true;
			entry.player.npc = true;
			entry.player.edition = structures::edition_java;
			entry.player.platform = bedrock_platform_java;
			entry.title.assign(parts[2]);
			entry.rows = 3u;
			std::from_chars(parts[3].data(), parts[3].data() + parts[3].size(), entry.rows);
			entry.rows = std::clamp(entry.rows, 1u, max_menu_rows);
		}
	}
	/*
	//=====================================================================================
	*/
	void npc_c::add_item(std::string_view line)
	{
		auto parts{ split(line, '|') };
		auto slot{ max_menu_rows * 9u };

		if (parts.size() >= 5u && npcs.size())
		{
			std::from_chars(parts[0].data(), parts[0].data() + parts[0].size(), slot);
		}

		if (npcs.size() && slot < npcs.back().rows * 9u)
		{
			auto& item{ npcs.back().items.emplace_back() };
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

			std::sort(npcs.back().items.begin(), npcs.back().items.end(), [](const auto& a, const auto& b) { return a.slot < b.slot; });
		}
	}
	/*
	//=====================================================================================
	*/
	void npc_c::run(structures::player_s& player, std::string_view action)
	{
		auto words{ split(action, ' ') };
		auto verb{ words.size() ? words[0] : std::string_view{} };
		auto rest{ action.size() > verb.size() ? trim(action.substr(verb.size())) : std::string_view{} };

		utils::log.write(color_play, "[npc](action) '%s' %.*s", player.name.c_str(), static_cast<std::int32_t>(action.size()), action.data());

		if (verb == "message")
		{
			chat.notify(player, rest);
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
	}
	/*
	//=====================================================================================
	*/
	std::FILE* npc_c::create_default(const std::string& path) const
	{
		if (auto file{ std::fopen(path.c_str(), "wb") }; file)
		{
			std::fprintf(file, "# npcs at spawn, hit or click one to open its menu\n");
			std::fprintf(file, "# npc=<name>|<x> <y> <z> <yaw>|<menu title>|<rows 1-6>\n");
			std::fprintf(file, "# item=<slot>|<item id>|<name>|<lore line;lore line>|<action>\n");
			std::fprintf(file, "# actions: message <text> | transfer <host> <port> | close\n");
			std::fprintf(file, "# colour codes like \xC2\xA7" "a work in names, titles and lore\n\n");
			std::fprintf(file, "npc=SMP|6.5 58 -8.5 -45|\xC2\xA7" "2Survival SMP|3\n");
			std::fprintf(file, "item=11|minecraft:grass_block|\xC2\xA7" "aJoin the SMP|\xC2\xA7" "7Survive and build with friends;\xC2\xA7" "eClick to play|message the smp world is opening soon\n");
			std::fprintf(file, "item=13|minecraft:compass|\xC2\xA7" "bRandom Teleport|\xC2\xA7" "7Start somewhere fresh;\xC2\xA7" "eClick to go|message random teleport opens with the smp\n");
			std::fprintf(file, "item=15|minecraft:red_bed|\xC2\xA7" "dHome|\xC2\xA7" "7Back to your bed;\xC2\xA7" "eClick to go home|message homes open with the smp\n\n");
			std::fprintf(file, "npc=Info|18.5 58 -8.5 45|\xC2\xA7" "6Server Info|3\n");
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
	std::vector<std::string_view> npc_c::split(std::string_view text, char separator)
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
	std::string_view npc_c::trim(std::string_view text)
	{
		auto start{ text.find_first_not_of(" \t\r\n") };
		auto end{ text.find_last_not_of(" \t\r\n") };

		return (start == std::string_view::npos) ? std::string_view{} : text.substr(start, end - start + 1u);
	}
}

//=====================================================================================
