
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	items_c items;

	void items_c::load()
	{
		std::unordered_map<std::string, std::int32_t> ids_26_2{};
		std::unordered_map<std::string, std::int32_t> ids_1_8{};
		std::unordered_map<std::string, std::uint16_t> drop_names{};

		for (const auto& row : read_columns(functions::get_layout(structures::family_26_3).item_path))
		{
			lookup.emplace(row[0], static_cast<std::uint16_t>(names.size()));
			names.push_back(row[0]);
		}

		for (const auto& row : read_columns(functions::get_layout(structures::family_26_2).item_path))
		{
			ids_26_2.emplace(row[0], static_cast<std::int32_t>(ids_26_2.size()));
		}

		for (const auto& row : read_columns(v1_8_item_path))
		{
			ids_1_8.emplace(row[0], (std::atoi(row[1].c_str()) << 16) | (std::atoi(row[2].c_str()) & 0xFFFF));
		}

		java[structures::family_26_2].resize(names.size(), modern_item_fallback);
		java[structures::family_26_3].resize(names.size(), modern_item_fallback);
		java[structures::family_1_8].resize(names.size(), v1_8_item_fallback);
		stacks.resize(names.size(), default_stack);
		block_of.resize(names.size(), kind_none);

		for (auto item{ 0u }; item < names.size(); ++item)
		{
			auto found_26_2{ ids_26_2.find(names[item]) };
			auto found_1_8{ ids_1_8.find(names[item]) };

			java[structures::family_26_3][item] = static_cast<std::int32_t>(item);
			java[structures::family_26_2][item] = found_26_2 != ids_26_2.end() ? found_26_2->second : modern_item_fallback;
			java[structures::family_1_8][item] = found_1_8 != ids_1_8.end() ? found_1_8->second : v1_8_item_fallback;
			block_of[item] = blocks.find_name(names[item]);
			reverse[structures::family_26_3].emplace(java[structures::family_26_3][item], static_cast<std::uint16_t>(item));

			if (found_26_2 != ids_26_2.end())
			{
				reverse[structures::family_26_2].emplace(found_26_2->second, static_cast<std::uint16_t>(item));
			}

			if (found_1_8 != ids_1_8.end())
			{
				reverse[structures::family_1_8].emplace(found_1_8->second, static_cast<std::uint16_t>(item));
			}
		}

		for (const auto& row : read_columns(stacks_path))
		{
			if (auto found{ lookup.find(row[0]) }; found != lookup.end())
			{
				stacks[found->second] = static_cast<std::uint8_t>(std::atoi(row[1].c_str()));
			}
		}

		for (const auto& row : read_columns(drops_path))
		{
			if (auto found{ lookup.find(row[1]) }; found != lookup.end())
			{
				drop_names.emplace(row[0], found->second);
			}
		}

		drops.resize(blocks.get_kind_count(), 0u);

		for (auto kind{ 0u }; kind < drops.size(); ++kind)
		{
			if (auto found{ drop_names.find(blocks.get_name(static_cast<std::uint16_t>(kind))) }; found != drop_names.end())
			{
				drops[kind] = found->second;
			}
		}

		utils::log.write(color_server, "[items](load) %zu items, %zu block drops", names.size(), drop_names.size());
	}
	/*
	//=====================================================================================
	*/
	std::int32_t items_c::get_java(structures::family_e family, std::uint16_t item) const
	{
		return java[family][item];
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t items_c::find_java(structures::family_e family, std::int32_t id) const
	{
		auto item{ std::uint16_t{ 0u } };

		if (auto found{ reverse[family].find(id) }; found != reverse[family].end())
		{
			item = found->second;
		}

		else if (auto plain{ reverse[family].find(id & ~0xFFFF) }; family == structures::family_1_8 && plain != reverse[family].end())
		{
			item = plain->second;
		}

		return item;
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t items_c::get_stack(std::uint16_t item) const
	{
		return stacks[item];
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t items_c::get_drop(std::uint16_t kind) const
	{
		return kind < drops.size() ? drops[kind] : 0u;
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t items_c::get_block(std::uint16_t item) const
	{
		return block_of[item];
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t items_c::find_name(std::string_view name) const
	{
		auto full{ name.starts_with("minecraft:") ? std::string{ name } : std::string{ "minecraft:" }.append(name) };
		auto found{ lookup.find(full) };
		return found != lookup.end() ? found->second : 0u;
	}
	/*
	//=====================================================================================
	*/
	const std::string& items_c::get_name(std::uint16_t item) const
	{
		return names[item];
	}
	/*
	//=====================================================================================
	*/
	std::string items_c::get_label(std::uint16_t item) const
	{
		auto label{ names[item].substr(names[item].find(':') + 1u) };
		std::replace(label.begin(), label.end(), '_', ' ');
		return label;
	}
	/*
	//=====================================================================================
	*/
	bool items_c::matches(std::uint16_t item, std::string_view filter) const
	{
		auto label{ get_label(item) };
		std::string wanted{ filter };
		std::replace(wanted.begin(), wanted.end(), '_', ' ');
		std::transform(wanted.begin(), wanted.end(), wanted.begin(), [](char value) { return static_cast<char>(std::tolower(static_cast<std::uint8_t>(value))); });
		return label.find(wanted) != std::string::npos;
	}
	/*
	//=====================================================================================
	*/
	std::vector<std::array<std::string, 3u>> items_c::read_columns(const char* path)
	{
		std::vector<std::array<std::string, 3u>> rows{};

		if (auto file{ std::fopen(functions::get_local_path(path).c_str(), "rb") }; file)
		{
			std::array<char, 256u> line{};
			std::array<std::array<char, 128u>, 3u> words{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				words = {};

				if (line[0] != '#' && std::sscanf(line.data(), "%127s %127s %127s", words[0].data(), words[1].data(), words[2].data()) >= 1)
				{
					rows.push_back({ words[0].data(), words[1].data(), words[2].data() });
				}
			}

			std::fclose(file);
		}

		return rows;
	}
}

//=====================================================================================
