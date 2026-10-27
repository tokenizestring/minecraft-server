
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class edits_c
	{
	public:

		bool set(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind);
		std::shared_ptr<const std::vector<structures::edit_s>> get(structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z) const;
		std::uint16_t find(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z) const;
		void clear(structures::world_e target);

		static std::uint64_t get_key(std::int32_t chunk_x, std::int32_t chunk_z);

	private:

		std::array<std::unordered_map<std::uint64_t, std::shared_ptr<const std::vector<structures::edit_s>>>, structures::world_count> chunks;
	};

	extern edits_c edits;
}

//=====================================================================================
