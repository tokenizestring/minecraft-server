
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class edits_c
	{
	public:

		void load();
		bool set(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind, std::uint16_t stamp);
		std::shared_ptr<const std::vector<structures::edit_s>> get(structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z) const;
		std::uint16_t find(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z) const;
		structures::edit_s find_edit(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z) const;
		void clear(structures::world_e target);
		void tick(std::uint64_t now);
		void shutdown();

		static std::uint64_t get_key(std::int32_t chunk_x, std::int32_t chunk_z);

	private:

		using snapshot_t = std::array<std::unordered_map<std::uint64_t, std::shared_ptr<const std::vector<structures::edit_s>>>, structures::world_count>;

		static void write(const snapshot_t& snapshot, std::uint64_t seed);

		snapshot_t chunks;
		std::thread writer;
		std::atomic<bool> writing;
		bool dirty;
		std::uint64_t last_save;
	};

	extern edits_c edits;
}

//=====================================================================================
