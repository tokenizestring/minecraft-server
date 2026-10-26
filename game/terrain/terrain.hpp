
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class terrain_c
	{
	public:

		void load();
		const structures::terrain_section_s* get_section(std::int32_t chunk_x, std::int32_t section_y, std::int32_t chunk_z) const;
		const structures::terrain_state_s& get_state(std::uint32_t index) const;
		const std::uint16_t* get_palette(const structures::terrain_section_s& section) const;
		const std::uint64_t* get_longs(const structures::terrain_section_s& section) const;
		std::uint32_t get_block(const structures::terrain_section_s& section, std::uint32_t index) const;
		bool is_solid(const structures::terrain_section_s& section, std::uint32_t index) const;

	private:

		template <typename type_t>
		static bool read_array(structures::reader_s& in, std::vector<type_t>& out, std::size_t count);

		std::vector<structures::terrain_state_s> states;
		std::vector<structures::terrain_section_s> sections;
		std::vector<structures::terrain_column_s> columns;
		std::vector<std::uint16_t> palettes;
		std::vector<std::uint64_t> longs;
		std::vector<std::uint32_t> refs;
		std::int32_t min_chunk_x;
		std::int32_t min_chunk_z;
		std::int32_t chunks_x;
		std::int32_t chunks_z;
		std::int32_t min_section_y;
		std::int32_t sections_y;
	};

	extern terrain_c terrain;
}

//=====================================================================================
