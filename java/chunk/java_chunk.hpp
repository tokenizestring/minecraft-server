
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_chunk_c
	{
	public:

		void build();
		void send_view(structures::client_s& client);
		void update_view(structures::client_s& client);
		void clear_view(structures::client_s& client);
		void reset_world(structures::world_e target);

	private:

		void send_center(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void send_column(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void send_forget(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void trim_cache();
		std::shared_ptr<const std::vector<std::uint8_t>> get_column(structures::family_e family, structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z);
		std::shared_ptr<const std::vector<std::uint8_t>> build_column(structures::family_e family, structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z);
		void write_generated(std::vector<std::uint8_t>& out, structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);
		void write_generated_1_8(std::vector<std::uint8_t>& out, std::int32_t chunk_x, std::int32_t chunk_z);
		void write_generated_section(std::vector<std::uint8_t>& out, structures::family_e family, const std::uint8_t* blocks) const;

		static void write_column(std::vector<std::uint8_t>& out, structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);
		static void write_heightmaps(std::vector<std::uint8_t>& out, const std::array<std::uint32_t, 256u>& heights);
		static void write_section(std::vector<std::uint8_t>& out, structures::family_e family, const structures::terrain_section_s* section);
		static void write_light(std::vector<std::uint8_t>& out, const structures::layout_s& layout, std::uint32_t lit);
		static void write_chunk_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z);
		static void write_empty_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z);
		static void load_blocks(std::array<std::uint32_t, structures::block_count>& ids, const char* path);
		static std::int32_t get_chunk(std::double_t position);

		std::array<std::array<std::unordered_map<std::uint64_t, structures::chunk_cache_s>, structures::family_count>, structures::world_count> cache;
		std::array<std::array<std::uint32_t, structures::block_count>, structures::family_count> block_ids;
		std::vector<std::pair<std::int32_t, std::int32_t>> offsets;
		std::vector<std::uint8_t> packet;
		std::vector<std::uint8_t> sections;
		std::size_t cache_bytes;
		std::uint64_t cache_clock;
	};

	extern java_chunk_c java_chunk;
}

//=====================================================================================
