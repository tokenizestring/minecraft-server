
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class light_c
	{
	public:

		std::uint32_t compute_sky(std::int32_t chunk_x, std::int32_t chunk_z);
		std::uint32_t compute_generated(const std::uint8_t* blocks, std::int32_t highest);
		const std::uint8_t* get_section(std::uint32_t section) const;

	private:

		void pack(std::uint32_t count);
		void load_filters(std::int32_t chunk_x, std::int32_t chunk_z, std::uint32_t height);
		void spread(std::uint32_t height);

		std::vector<std::uint8_t> filters;
		std::vector<std::uint8_t> levels;
		std::vector<std::uint32_t> queue;
		std::vector<std::uint8_t> nibbles;
	};

	extern light_c light;
}

//=====================================================================================
