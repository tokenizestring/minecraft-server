
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class light_c
	{
	public:

		using around_t = std::array<const std::vector<structures::edit_s>*, 9u>;

		std::uint32_t compute_sky(std::int32_t chunk_x, std::int32_t chunk_z);
		std::uint32_t compute_generated(const std::uint8_t* column, std::int32_t highest, const around_t& around);
		const std::uint8_t* get_section(std::uint32_t section) const;
		const std::uint8_t* get_block_section(std::uint32_t section) const;
		std::uint64_t get_block_mask() const;

	private:

		void pack(std::uint32_t count);
		void load_filters(std::int32_t chunk_x, std::int32_t chunk_z, std::uint32_t height);
		void load_edits(const around_t& around, std::uint32_t height);
		void spread(std::uint32_t height);
		void flood(std::vector<std::uint8_t>& target, std::uint32_t height);

		std::vector<std::uint8_t> filters;
		std::vector<std::uint8_t> emits;
		std::vector<std::uint8_t> levels;
		std::vector<std::uint8_t> block_levels;
		std::vector<std::uint32_t> queue;
		std::vector<std::uint8_t> nibbles;
		std::vector<std::uint8_t> block_nibbles;
		std::vector<structures::edit_s> no_edits;
		std::uint64_t block_mask;
	};

	extern light_c light;
}

//=====================================================================================
