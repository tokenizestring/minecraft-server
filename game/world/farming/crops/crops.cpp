
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	crops_c crops;

	void crops_c::load()
	{
		auto rows{ read_rows(crops_path) };
		random = functions::make_seed() | 1ui64;

		for (const auto& words : rows)
		{
			if (words[0] == "crop" && words.size() == crop_row_words && list.size() < max_crops)
			{
				add_crop(words);
			}
		}

		link_kinds(rows);

		utils::log.write(color_server, "[crops](load) %zu crops growing %llux vanilla speed, %zu tillable kinds, %zu hoes", list.size(), config.crop_growth, static_cast<std::size_t>(std::count_if(till_to.begin(), till_to.end(), [](auto kind) { return kind != kind_none; })), static_cast<std::size_t>(std::count(hoes.begin(), hoes.end(), 1u)));
	}
	/*
	//=====================================================================================
	*/
	bool crops_c::is_crop(std::uint16_t kind) const
	{
		return kind < crop_of.size() && crop_of[kind] != crop_none;
	}
	/*
	//=====================================================================================
	*/
	bool crops_c::is_growing(std::uint16_t kind) const
	{
		return is_crop(kind) && list[crop_of[kind]].first == kind;
	}
	/*
	//=====================================================================================
	*/
	bool crops_c::is_hoe(std::uint16_t item) const
	{
		return item < hoes.size() && hoes[item] != 0u;
	}
	/*
	//=====================================================================================
	*/
	bool crops_c::can_plant(std::uint16_t kind, std::uint16_t below) const
	{
		return is_crop(kind) && below < soil_of.size() && soil_of[below] != soil_none && soil_of[below] == list[crop_of[kind]].soil;
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t crops_c::get_till(std::uint16_t kind) const
	{
		return kind < till_to.size() ? till_to[kind] : kind_none;
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t crops_c::get_ripe(std::uint16_t kind) const
	{
		return is_growing(kind) ? static_cast<std::uint16_t>(kind + list[crop_of[kind]].max_age) : kind;
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t crops_c::get_settled(std::uint16_t kind, std::uint32_t elapsed) const
	{
		return is_growing(kind) && get_age(list[crop_of[kind]], elapsed) >= list[crop_of[kind]].max_age ? get_ripe(kind) : kind;
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t crops_c::get_shown(const structures::edit_s& edit, std::uint32_t clock) const
	{
		return is_growing(edit.kind) ? static_cast<std::uint16_t>(edit.kind + get_age(list[crop_of[edit.kind]], get_elapsed(edit.stamp, clock))) : edit.kind;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t crops_c::get_ripen(const structures::edit_s& edit, std::uint32_t clock) const
	{
		auto due{ growth_never };

		if (is_growing(edit.kind))
		{
			const auto& crop{ list[crop_of[edit.kind]] };
			auto elapsed{ get_elapsed(edit.stamp, clock) };
			auto age{ get_age(crop, elapsed) };
			auto rate{ ticks_per_second * config.crop_growth };

			if (age < crop.max_age)
			{
				due = clock - elapsed + static_cast<std::uint32_t>(((age + 1ui64) * crop.stage_ticks + rate - 1ui64) / rate);
			}
		}

		return due;
	}
	/*
	//=====================================================================================
	*/
	std::array<structures::item_stack_s, 2u> crops_c::get_loot(std::uint16_t kind)
	{
		std::array<structures::item_stack_s, 2u> loot{};
		auto crop{ is_crop(kind) ? &list[crop_of[kind]] : nullptr };

		if (crop && age_of[kind] >= crop->max_age)
		{
			loot[0] = { crop->produce, roll(crop->produce_count, crop->probability) };
			loot[1] = { crop->extra, crop->extra ? roll(crop->extra_count, crop->probability) : std::uint8_t{ 0u } };
		}

		else if (crop)
		{
			loot[0] = { crop->seed, 1u };
		}

		else
		{
			loot[0] = { items.get_drop(kind), 1u };
			loot[1] = { kind < chance_item.size() ? chance_item[kind] : std::uint16_t{ 0u }, static_cast<std::uint8_t>(kind < chance_odds.size() && next_unit() < chance_odds[kind] ? 1u : 0u) };
		}

		return loot;
	}
	/*
	//=====================================================================================
	*/
	void crops_c::add_crop(const std::vector<std::string>& words)
	{
		structures::crop_s crop{};
		auto max_age{ static_cast<std::uint32_t>(std::max(1, std::atoi(words[2].c_str()))) };
		auto like{ blocks.find_name(words[1]) };
		auto count{ [&words](std::size_t index) { return static_cast<std::uint8_t>(std::atoi(words[index].c_str())); } };

		crop.max_age = static_cast<std::uint8_t>(max_age);
		crop.stage_ticks = static_cast<std::uint32_t>(std::max(1, std::atoi(words[3].c_str())));
		crop.soil = get_soil(words[8]);
		crop.seed = items.find_name(words[9]);
		crop.produce = items.find_name(words[10]);
		crop.extra = items.find_name(words[14]);
		crop.produce_count = { count(11u), count(12u), count(13u) };
		crop.extra_count = { count(15u), count(16u), count(17u) };
		crop.probability = static_cast<std::float_t>(std::atof(words[18].c_str()));

		for (auto age{ 0u }; age <= max_age; ++age)
		{
			std::array<std::uint32_t, structures::family_count> states{};
			states[structures::family_26_2] = static_cast<std::uint32_t>(std::atoi(words[4].c_str())) + age;
			states[structures::family_26_3] = static_cast<std::uint32_t>(std::atoi(words[5].c_str())) + age;
			states[structures::family_1_8] = (static_cast<std::uint32_t>(std::atoi(words[6].c_str())) << 4u) | ((age * static_cast<std::uint32_t>(std::atoi(words[7].c_str())) / max_age) & 15u);

			auto kind{ blocks.add_kind(words[1] + "[age=" + std::to_string(age) + "]", states, like) };
			crop.first = age ? crop.first : kind;
		}

		items.set_block(crop.produce, kind_none);
		items.set_block(crop.seed, crop.first);
		list.push_back(crop);
		crop_names.push_back(words[1]);
	}
	/*
	//=====================================================================================
	*/
	void crops_c::link_kinds(const std::vector<std::vector<std::string>>& rows)
	{
		std::unordered_map<std::string, std::vector<std::uint16_t>> by_name{};
		auto count{ blocks.get_kind_count() };

		crop_of.assign(count, crop_none);
		age_of.assign(count, 0u);
		soil_of.assign(count, soil_none);
		till_to.assign(count, kind_none);
		chance_item.assign(count, 0u);
		chance_odds.assign(count, 0.0f);

		for (auto kind{ 0u }; kind < count; ++kind)
		{
			const auto& name{ blocks.get_name(static_cast<std::uint16_t>(kind)) };
			by_name[name.substr(0u, name.find('['))].push_back(static_cast<std::uint16_t>(kind));
		}

		for (auto index{ 0u }; index < list.size(); ++index)
		{
			auto first{ static_cast<std::int64_t>(blocks.get_state(structures::family_26_3, list[index].first)) };

			for (auto kind : by_name[crop_names[index]])
			{
				crop_of[kind] = static_cast<std::uint8_t>(index);
				age_of[kind] = static_cast<std::uint8_t>(std::clamp<std::int64_t>(static_cast<std::int64_t>(blocks.get_state(structures::family_26_3, kind)) - first, 0i64, list[index].max_age));
			}
		}

		for (auto index{ 0u }; index < soil_names.size(); ++index)
		{
			for (auto kind : by_name[soil_names[index]])
			{
				soil_of[kind] = static_cast<std::uint8_t>(index + 1u);
			}
		}

		for (const auto& words : rows)
		{
			if (words[0] == "till" && words.size() == 3u)
			{
				for (auto kind : by_name[words[1]])
				{
					till_to[kind] = blocks.find_name(words[2]);
				}
			}

			else if (words[0] == "chance" && words.size() == 4u)
			{
				for (auto kind : by_name[words[1]])
				{
					chance_item[kind] = items.find_name(words[2]);
					chance_odds[kind] = static_cast<std::float_t>(std::atof(words[3].c_str()));
				}
			}

			else if (auto item{ words.size() == 2u && words[0] == "hoe" ? items.find_name(words[1]) : std::uint16_t{ 0u } }; item)
			{
				hoes.resize(std::max<std::size_t>(hoes.size(), item + 1u), 0u);
				hoes[item] = 1u;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t crops_c::get_soil(const std::string& name)
	{
		auto found{ std::find(soil_names.begin(), soil_names.end(), name) };

		if (found == soil_names.end())
		{
			soil_names.push_back(name);
			found = soil_names.end() - 1;
		}

		return static_cast<std::uint8_t>(found - soil_names.begin() + 1);
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t crops_c::roll(const std::array<std::uint8_t, 3u>& count, std::float_t probability)
	{
		auto total{ static_cast<std::uint32_t>(count[0]) + (count[1] > count[0] ? static_cast<std::uint32_t>(next() % (count[1] - count[0] + 1u)) : 0u) };

		for (auto trial{ 0u }; trial < count[2]; ++trial)
		{
			total += next_unit() < probability ? 1u : 0u;
		}

		return static_cast<std::uint8_t>(total);
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t crops_c::next()
	{
		random ^= random << 13u;
		random ^= random >> 7u;
		random ^= random << 17u;

		return random;
	}
	/*
	//=====================================================================================
	*/
	std::float_t crops_c::next_unit()
	{
		return static_cast<std::float_t>(next() >> 40u) / 16777216.0f;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t crops_c::get_elapsed(std::uint16_t stamp, std::uint32_t clock)
	{
		return (clock - stamp) & crop_stamp_mask;
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t crops_c::get_age(const structures::crop_s& crop, std::uint64_t elapsed)
	{
		return static_cast<std::uint8_t>(std::min<std::uint64_t>(crop.max_age, elapsed * ticks_per_second * config.crop_growth / crop.stage_ticks));
	}
	/*
	//=====================================================================================
	*/
	std::vector<std::vector<std::string>> crops_c::read_rows(const char* path)
	{
		std::vector<std::vector<std::string>> rows{};

		if (auto file{ std::fopen(functions::get_local_path(path).c_str(), "rb") }; file)
		{
			std::array<char, 512u> line{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::vector<std::string> words{};
				std::string_view text{ line.data() };
				auto start{ text.find_first_not_of(row_blanks) };

				while (start != std::string_view::npos)
				{
					auto end{ text.find_first_of(row_blanks, start) };
					words.emplace_back(text.substr(start, end == std::string_view::npos ? end : end - start));
					start = text.find_first_not_of(row_blanks, end);
				}

				if (words.size() && words[0][0] != '#')
				{
					rows.push_back(std::move(words));
				}
			}

			std::fclose(file);
		}

		return rows;
	}
}

//=====================================================================================
