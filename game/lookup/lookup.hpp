
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class lookup_c
	{
	public:

		std::uint16_t get(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z);
		std::int32_t get_top(std::int32_t x, std::int32_t z, std::uint16_t& kind);
		bool is_land(std::int32_t x, std::int32_t z);

	private:

		void sync();

		struct column_s
		{
			std::uint64_t last_use;
			std::unique_ptr<std::array<std::uint8_t, column_volume>> blocks;
		};

		const std::array<std::uint8_t, column_volume>& get_column(std::int32_t chunk_x, std::int32_t chunk_z);

		generator_c generator;
		std::uint64_t seed;
		std::uint64_t clock;
		std::unordered_map<std::uint64_t, column_s> columns;
	};

	extern lookup_c lookup;
}

//=====================================================================================
