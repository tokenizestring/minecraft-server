
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	blocks_c blocks;

	void blocks_c::load()
	{
		auto count{ 0u };
		kind_names.resize(placed_kind_base);

		if (auto file{ std::fopen(functions::get_local_path(gen_block_path).c_str(), "rb") }; file)
		{
			std::array<char, 512u> line{};
			std::array<char, 64u> key{};
			std::array<char, 256u> state{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::uint32_t id_26_2{};
				std::uint32_t id_26_3{};
				std::uint32_t legacy{};
				std::uint32_t filter{};
				std::uint32_t fluid{};

				if (count < structures::block_count && std::sscanf(line.data(), "%63s %255s %u %u %u %u %u", key.data(), state.data(), &id_26_2, &id_26_3, &legacy, &filter, &fluid) == 7)
				{
					ids[structures::family_26_2][count] = id_26_2;
					ids[structures::family_26_3][count] = id_26_3;
					ids[structures::family_1_8][count] = legacy;
					filters[count] = static_cast<std::uint8_t>(filter);
					fluids[count] = fluid != 0u;
					names.emplace(std::string{ state.data(), std::strcspn(state.data(), "[") }, static_cast<std::uint16_t>(count));
					kind_names[count].assign(state.data(), std::strcspn(state.data(), "["));
					count += 1u;
				}
			}

			std::fclose(file);
		}

		load_placed();
		load_light();

		if (count == structures::block_count)
		{
			utils::log.write(color_server, "[blocks](load) %u generated block states, %zu placeable blocks for 26.2, 26.3 and 1.8", count, placed[structures::family_26_3].size());
		}

		else
		{
			utils::log.write(color_error, "[blocks](load) '%s' has %u of %u blocks, generated worlds will be wrong", gen_block_path, count, static_cast<std::uint32_t>(structures::block_count));
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t blocks_c::get_java(structures::family_e family, std::uint8_t block) const
	{
		return ids[family][block];
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t blocks_c::get_state(structures::family_e family, std::uint16_t kind) const
	{
		return kind < placed_kind_base ? ids[family][kind] : placed[family][kind - placed_kind_base];
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t blocks_c::get_filter(std::uint8_t block) const
	{
		return filters[block];
	}
	/*
	//=====================================================================================
	*/
	bool blocks_c::is_fluid(std::uint8_t block) const
	{
		return fluids[block];
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t blocks_c::get_emission(std::uint16_t kind) const
	{
		return kind < kind_emission.size() ? kind_emission[kind] : 0u;
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t blocks_c::get_opacity(std::uint16_t kind) const
	{
		return kind < kind_opacity.size() ? kind_opacity[kind] : static_cast<std::uint8_t>(light_level_max);
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t blocks_c::get_state_emission(std::uint32_t state) const
	{
		return state < state_emission.size() ? state_emission[state] : 0u;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t blocks_c::get_light_state(structures::family_e family, std::uint8_t level) const
	{
		return get_state(family, light_kind) - (light_level_max - std::min<std::uint32_t>(level, light_level_max)) * 2u;
	}
	/*
	//=====================================================================================
	*/
	void blocks_c::load_light()
	{
		std::vector<std::uint8_t> state_opacity{};

		if (auto file{ std::fopen(functions::get_local_path(light_path).c_str(), "rb") }; file)
		{
			std::uint32_t state{};
			std::uint32_t level{};
			std::uint32_t opacity{};

			while (std::fscanf(file, "%u %u %u", &state, &level, &opacity) == 3)
			{
				state_emission.resize(std::max<std::size_t>(state_emission.size(), state + 1u), 0u);
				state_opacity.resize(std::max<std::size_t>(state_opacity.size(), state + 1u), static_cast<std::uint8_t>(light_level_max));
				state_emission[state] = static_cast<std::uint8_t>(std::min<std::uint32_t>(level, light_level_max));
				state_opacity[state] = static_cast<std::uint8_t>(std::min<std::uint32_t>(opacity, light_level_max));
			}

			std::fclose(file);
		}

		light_kind = find_name("minecraft:light");
		kind_emission.assign(kind_names.size(), 0u);
		kind_opacity.assign(kind_names.size(), static_cast<std::uint8_t>(light_level_max));

		for (auto kind{ 0u }; kind < kind_names.size(); ++kind)
		{
			auto state{ get_state(structures::family_26_3, static_cast<std::uint16_t>(kind)) };
			kind_emission[kind] = get_state_emission(state);
			kind_opacity[kind] = kind < placed_kind_base ? filters[kind] : (state < state_opacity.size() ? state_opacity[state] : static_cast<std::uint8_t>(light_level_max));
		}

		utils::log.write(color_server, "[blocks](light) %zu light states, %zu glowing kinds", state_emission.size(), static_cast<std::size_t>(std::count_if(kind_emission.begin(), kind_emission.end(), [](auto level) { return level != 0u; })));
	}
	/*
	//=====================================================================================
	*/
	void blocks_c::load_placed()
	{
		std::unordered_map<std::string, std::uint32_t> lookup_26_2{};
		std::unordered_map<std::string, std::uint32_t> lookup_1_8{};

		for (const auto& [name, id] : read_table(block_path_26_2, false))
		{
			lookup_26_2.emplace(name, id);
		}

		for (const auto& [name, state] : read_table(block_path_1_8, true))
		{
			lookup_1_8.emplace(name, state);
		}

		for (const auto& [name, state] : read_table(v1_8_item_path, true))
		{
			if ((state >> 4u) < v1_8_block_items)
			{
				lookup_1_8.emplace(name, state);
			}
		}

		for (const auto& [name, id] : read_table(block_path_26_3, false))
		{
			auto found_26_2{ lookup_26_2.find(name) };
			auto found_1_8{ lookup_1_8.find(name) };

			names.emplace(name, static_cast<std::uint16_t>(placed_kind_base + placed[structures::family_26_3].size()));
			kind_names.resize(placed_kind_base + placed[structures::family_26_3].size());
			kind_names.push_back(name);
			placed[structures::family_26_3].push_back(id);
			placed[structures::family_26_2].push_back(found_26_2 != lookup_26_2.end() ? found_26_2->second : ids[structures::family_26_2][structures::block_stone]);
			placed[structures::family_1_8].push_back(found_1_8 != lookup_1_8.end() ? found_1_8->second : v1_8_stone_state);
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t blocks_c::find_name(const std::string& name) const
	{
		auto found{ names.find(name) };
		return found != names.end() && name != "minecraft:air" ? found->second : kind_none;
	}
	/*
	//=====================================================================================
	*/
	const std::string& blocks_c::get_name(std::uint16_t kind) const
	{
		return kind_names[kind];
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t blocks_c::get_kind_count() const
	{
		return static_cast<std::uint32_t>(kind_names.size());
	}
	/*
	//=====================================================================================
	*/
	std::vector<std::pair<std::string, std::uint32_t>> blocks_c::read_table(const char* path, bool legacy)
	{
		std::vector<std::pair<std::string, std::uint32_t>> table{};

		if (auto file{ std::fopen(functions::get_local_path(path).c_str(), "rb") }; file)
		{
			std::array<char, 256u> line{};
			std::array<char, 128u> name{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::uint32_t id{};
				std::uint32_t meta{};
				auto fields{ std::sscanf(line.data(), "%127s %u %u", name.data(), &id, &meta) };

				if (fields == 3 && legacy)
				{
					table.emplace_back(name.data(), (id << 4u) | (meta & 15u));
				}

				else if (fields == 2)
				{
					table.emplace_back(name.data(), id);
				}

				else if (fields == 1)
				{
					table.emplace_back(name.data(), static_cast<std::uint32_t>(table.size()));
				}
			}

			std::fclose(file);
		}

		return table;
	}
}

//=====================================================================================
