
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class crops_c
	{
	public:

		void load();
		bool is_crop(std::uint16_t kind) const;
		bool is_growing(std::uint16_t kind) const;
		bool is_hoe(std::uint16_t item) const;
		bool can_plant(std::uint16_t kind, std::uint16_t below) const;
		std::uint16_t get_till(std::uint16_t kind) const;
		std::uint16_t get_ripe(std::uint16_t kind) const;
		std::uint16_t get_settled(std::uint16_t kind, std::uint32_t elapsed) const;
		std::uint16_t get_shown(const structures::edit_s& edit, std::uint32_t clock) const;
		std::uint32_t get_ripen(const structures::edit_s& edit, std::uint32_t clock) const;
		std::array<structures::item_stack_s, 2u> get_loot(std::uint16_t kind);

	private:

		void add_crop(const std::vector<std::string>& words);
		void link_kinds(const std::vector<std::vector<std::string>>& rows);
		std::uint8_t get_soil(const std::string& name);
		std::uint8_t roll(const std::array<std::uint8_t, 3u>& count, std::float_t probability);
		std::uint64_t next();
		std::float_t next_unit();

		static std::uint32_t get_elapsed(std::uint16_t stamp, std::uint32_t clock);
		static std::uint8_t get_age(const structures::crop_s& crop, std::uint64_t elapsed);
		static std::vector<std::vector<std::string>> read_rows(const char* path);

		std::vector<structures::crop_s> list;
		std::vector<std::string> crop_names;
		std::vector<std::string> soil_names;
		std::vector<std::uint8_t> crop_of;
		std::vector<std::uint8_t> age_of;
		std::vector<std::uint8_t> soil_of;
		std::vector<std::uint16_t> till_to;
		std::vector<std::uint16_t> chance_item;
		std::vector<std::float_t> chance_odds;
		std::vector<std::uint8_t> hoes;
		std::uint64_t random;
	};

	extern crops_c crops;
}

//=====================================================================================
