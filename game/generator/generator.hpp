
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class generator_c
	{
	public:

		void set_seed(std::uint64_t seed);
		void generate(std::int32_t chunk_x, std::int32_t chunk_z);
		const std::uint8_t* get_blocks() const;
		std::int32_t get_top(std::uint32_t column) const;
		structures::spawn_s get_spawn() const;

		std::uint64_t get_seed() const;
		std::float_t noise2(std::float_t x, std::float_t z, std::uint64_t salt) const;
		std::float_t noise3(std::float_t x, std::float_t y, std::float_t z, std::uint64_t salt) const;
		std::float_t fractal2(std::float_t x, std::float_t z, std::uint32_t octaves, std::float_t frequency, std::uint64_t salt) const;
		std::int32_t surface_height(std::int32_t x, std::int32_t z) const;
		structures::biome_e biome_at(std::int32_t x, std::int32_t z) const;
		structures::biome_e biome_for(std::int32_t x, std::int32_t z, std::int32_t height) const;
		structures::biome_e get_biome(std::uint32_t column) const;
		std::int32_t get_ground(std::uint32_t column) const;
		bool site_clear(std::int32_t world_x, std::int32_t world_z) const;

		std::uint8_t get_block(std::int32_t x, std::int32_t y, std::int32_t z) const;
		void set_block(std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t block);
		bool inside(std::int32_t world_x, std::int32_t world_z) const;
		std::uint8_t peek(std::int32_t world_x, std::int32_t y, std::int32_t world_z) const;
		void solid(std::int32_t world_x, std::int32_t y, std::int32_t world_z, std::uint8_t block);
		void soft(std::int32_t world_x, std::int32_t y, std::int32_t world_z, std::uint8_t block);

	private:

		std::float_t gradient(std::int64_t x, std::int64_t z, std::uint64_t salt) const;
		std::float_t gradient3(std::int64_t x, std::int64_t y, std::int64_t z, std::uint64_t salt) const;
		std::float_t tunnel_band(std::float_t x, std::float_t y, std::float_t z, std::float_t horizontal, std::float_t vertical, std::float_t width, std::float_t fill, std::uint64_t salt) const;
		std::float_t carve_density(std::float_t x, std::float_t y, std::float_t z) const;
		bool lava_here(std::int32_t x, std::int32_t y, std::int32_t z) const;
		void sample_caves(std::int32_t highest);
		std::float_t cave_at(std::int32_t local_x, std::int32_t y, std::int32_t local_z) const;
		void fill_column(std::int32_t local_x, std::int32_t local_z);
		void surface_column(std::int32_t local_x, std::int32_t local_z, std::int32_t height, structures::biome_e biome);
		void place_veins(std::int32_t chunk_x, std::int32_t chunk_z, std::uint8_t stone, std::uint8_t deepslate, std::int32_t min_y, std::int32_t max_y, std::int32_t attempts, std::int32_t size, std::uint64_t salt);

		static std::float_t smooth(std::float_t t);
		static std::size_t index(std::int32_t x, std::int32_t y, std::int32_t z);

		std::array<std::uint8_t, column_volume> blocks;
		std::array<std::float_t, gen_cave_volume> caves;
		std::array<std::int32_t, 256u> heights;
		std::array<std::int32_t, 256u> grounds;
		std::array<std::int32_t, 256u> tops;
		std::array<structures::biome_e, 256u> biomes;
		std::uint64_t world_seed{ gen_default_seed };
		std::int32_t base_x{ 0 };
		std::int32_t base_z{ 0 };

		caverns_c caverns;
		features_c features;
		flora_c flora;
	};

	extern generator_c generator;
}

//=====================================================================================
