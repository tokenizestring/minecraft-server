
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class growth_c
	{
	public:

		void track(std::uint64_t key, const structures::edit_s& edit);
		void tick();
		std::size_t get_tracked() const;

	private:

		void schedule(const structures::growth_s& entry, std::uint32_t due, std::uint32_t clock);
		void ripen(const structures::growth_s& entry, std::uint32_t clock);

		std::array<std::vector<structures::growth_s>, growth_wheel_size> wheel;
		std::vector<structures::growth_s> firing;
		std::uint32_t cursor;
		std::size_t tracked;
	};

	extern growth_c growth;
}

//=====================================================================================
