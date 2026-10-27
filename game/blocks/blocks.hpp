
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class blocks_c
	{
	public:

		void load();
		std::uint32_t get_java(structures::family_e family, std::uint8_t block) const;
		std::uint32_t get_state(structures::family_e family, std::uint16_t kind) const;
		std::uint8_t get_filter(std::uint8_t block) const;
		bool is_fluid(std::uint8_t block) const;
		std::uint16_t find_item(structures::family_e family, std::int32_t item) const;
		std::uint16_t find_item_1_8(std::int32_t item, std::int32_t damage) const;

	private:

		void load_placed();
		std::uint16_t find_name(const std::string& name) const;

		static std::vector<std::pair<std::string, std::uint32_t>> read_table(const char* path, bool legacy);

		std::array<std::array<std::uint32_t, structures::block_count>, structures::family_count> ids;
		std::array<std::uint8_t, 256u> filters;
		std::array<bool, 256u> fluids;
		std::array<std::vector<std::uint32_t>, structures::family_count> placed;
		std::array<std::vector<std::uint16_t>, structures::family_count> items;
		std::unordered_map<std::uint32_t, std::uint16_t> items_1_8;
		std::unordered_map<std::string, std::uint16_t> names;
	};

	extern blocks_c blocks;
}

//=====================================================================================
