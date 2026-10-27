
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_builder_c
	{
	public:

		std::shared_ptr<const std::vector<std::uint8_t>> build(const structures::chunk_job_s& job);
		std::shared_ptr<const std::array<std::int16_t, 256u>> take_surface();
		std::uint32_t get_expires() const;

		static void write_empty_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z);

	private:

		void write_generated(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);
		void write_generated_1_8(std::int32_t chunk_x, std::int32_t chunk_z);
		void write_generated_section(std::vector<std::uint8_t>& out, structures::family_e family, const std::uint8_t* column) const;
		void write_placed_section(std::vector<std::uint8_t>& out, structures::family_e family);
		void place_edits();
		void make_surface();
		bool fill_kinds(std::uint32_t section);
		void write_terrain(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);
		void write_light(const structures::layout_s& layout, std::uint32_t lit);
		void frame();

		static void write_heightmaps(std::vector<std::uint8_t>& out, const std::array<std::uint32_t, 256u>& heights);
		static void write_terrain_section(std::vector<std::uint8_t>& out, structures::family_e family, const structures::terrain_section_s* section);
		static void write_platform_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z);

		generator_c generator;
		light_c light;
		deflate_c deflater;
		std::uint64_t seed;
		std::uint32_t clock;
		std::uint32_t expires;
		const std::vector<structures::edit_s>* overrides;
		light_c::around_t around;
		std::shared_ptr<const std::array<std::int16_t, 256u>> surface;
		std::vector<structures::edit_s> no_edits;
		std::array<std::uint16_t, section_volume> kinds;
		std::array<std::uint16_t, section_volume> slots;
		std::vector<std::uint16_t> palette;
		std::vector<std::uint8_t> packet;
		std::vector<std::uint8_t> sections;
		std::vector<std::uint8_t> compressed;
		std::vector<std::uint8_t> framed;
	};
}

//=====================================================================================
