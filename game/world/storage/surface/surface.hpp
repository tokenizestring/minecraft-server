
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class surface_c
	{
	public:

		void store(std::int32_t chunk_x, std::int32_t chunk_z, std::shared_ptr<const std::array<std::int16_t, 256u>> map);
		std::int16_t get(std::int32_t x, std::int32_t z) const;
		bool has(std::int32_t chunk_x, std::int32_t chunk_z) const;
		void change(std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind);
		void clear();

	private:

		void trim();

		std::unordered_map<std::uint64_t, std::shared_ptr<const std::array<std::int16_t, 256u>>> maps;
	};

	extern surface_c surface;
}

//=====================================================================================
