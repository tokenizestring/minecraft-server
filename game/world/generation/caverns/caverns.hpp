
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class generator_c;

	class caverns_c
	{
	public:

		void dress(generator_c* parent, std::int32_t chunk_x, std::int32_t chunk_z);

	private:

		void dress_cell(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t state, bool lush, bool dripping);
		void hang_dripstone(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::int32_t length);
		void raise_dripstone(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::int32_t length);
		void hang_cave_vine(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::int32_t length);
		void cave_pool(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint8_t liquid, std::uint64_t seed);
		void geode(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t seed);
		void rough_stone(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t seed);
		void rough_ceiling(std::int32_t local_x, std::int32_t y, std::int32_t local_z, std::uint64_t seed);

		static bool cave_solid(std::uint8_t block);
		static std::uint8_t floor_slab_for(std::uint8_t ground);
		static std::uint8_t ceiling_slab_for(std::uint8_t roof);

		generator_c* owner{ nullptr };
	};
}

//=====================================================================================
