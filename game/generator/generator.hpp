
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
		std::int32_t get_surface(std::int32_t x, std::int32_t z) const;
		std::uint32_t get_biome(std::int32_t x, std::int32_t z, std::int32_t surface) const;
		structures::spawn_s get_spawn() const;

	private:

		std::array<std::int32_t, 3u> find_spawn() const;

		void fill(std::int32_t chunk_x, std::int32_t chunk_z);
		void carve(std::int32_t chunk_x, std::int32_t chunk_z);
		void place_ores(std::int32_t chunk_x, std::int32_t chunk_z);
		void decorate(std::int32_t chunk_x, std::int32_t chunk_z);
		void place_trees(std::int32_t chunk_x, std::int32_t chunk_z);
		void place_tree(std::int32_t base_x, std::int32_t base_y, std::int32_t base_z, std::uint32_t kind, std::uint64_t& random);
		void put(std::int32_t x, std::int32_t y, std::int32_t z, std::uint8_t block, bool replace);
		std::double_t get_cave(std::int32_t x, std::int32_t y, std::int32_t z) const;

		static std::uint32_t hash(std::int32_t x, std::int32_t y, std::int32_t z, std::uint32_t seed);
		static std::uint64_t next(std::uint64_t& state);
		static std::double_t noise_2d(std::double_t x, std::double_t z, std::uint32_t seed);
		static std::double_t noise_3d(std::double_t x, std::double_t y, std::double_t z, std::uint32_t seed);
		static std::double_t fractal(std::double_t x, std::double_t z, std::uint32_t octaves, std::double_t frequency, std::uint32_t seed);
		static std::size_t index(std::int32_t x, std::int32_t y, std::int32_t z);

		std::array<std::uint8_t, column_volume> blocks;
		std::array<std::int32_t, 256u> surfaces;
		std::array<std::uint32_t, 256u> biomes;
		std::array<std::int32_t, 256u> tops;
		std::array<std::double_t, 5u * 5u * 49u> caves;
		std::uint64_t world_seed;
		std::uint32_t seed_low;
	};

	extern generator_c generator;
}

//=====================================================================================
