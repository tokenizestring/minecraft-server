
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
		std::uint8_t get_filter(std::uint8_t block) const;
		bool is_fluid(std::uint8_t block) const;

	private:

		std::array<std::array<std::uint32_t, structures::block_count>, structures::family_count> ids;
		std::array<std::uint8_t, 256u> filters;
		std::array<bool, 256u> fluids;
	};

	extern blocks_c blocks;
}

//=====================================================================================
