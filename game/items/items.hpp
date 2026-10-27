
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class items_c
	{
	public:

		void load();
		std::int32_t get_java(structures::family_e family, std::uint16_t item) const;
		std::uint16_t find_java(structures::family_e family, std::int32_t id) const;
		std::uint8_t get_stack(std::uint16_t item) const;
		std::uint16_t get_drop(std::uint16_t kind) const;
		std::uint16_t get_block(std::uint16_t item) const;
		std::uint16_t find_name(std::string_view name) const;
		const std::string& get_name(std::uint16_t item) const;
		std::string get_label(std::uint16_t item) const;
		bool matches(std::uint16_t item, std::string_view filter) const;

	private:

		static std::vector<std::array<std::string, 3u>> read_columns(const char* path);

		std::vector<std::string> names;
		std::unordered_map<std::string, std::uint16_t> lookup;
		std::array<std::vector<std::int32_t>, structures::family_count> java;
		std::array<std::unordered_map<std::int32_t, std::uint16_t>, structures::family_count> reverse;
		std::vector<std::uint8_t> stacks;
		std::vector<std::uint16_t> drops;
		std::vector<std::uint16_t> block_of;
	};

	extern items_c items;
}

//=====================================================================================
