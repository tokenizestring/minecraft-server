
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
		const std::vector<std::uint8_t>& get_1_8() const;
		void send_view(structures::client_s& client);
		void update_view(structures::client_s& client);

	private:

		void send_center(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void send_column(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void send_forget(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void trim_cache();
		std::shared_ptr<const std::vector<std::uint8_t>> get_column(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);
		std::shared_ptr<const std::vector<std::uint8_t>> build_column(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);

		static void write_column(std::vector<std::uint8_t>& out, structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z);
		static void write_heightmaps(std::vector<std::uint8_t>& out, std::int32_t chunk_x, std::int32_t chunk_z);
		static void write_section(std::vector<std::uint8_t>& out, structures::family_e family, const structures::terrain_section_s* section);
		static void write_chunk_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z);
		static std::int32_t get_chunk(std::double_t position);

		std::array<std::unordered_map<std::uint64_t, structures::chunk_cache_s>, structures::family_count> cache;
		std::vector<std::pair<std::int32_t, std::int32_t>> offsets;
		std::vector<std::uint8_t> world_1_8;
		std::vector<std::uint8_t> packet;
		std::size_t cache_bytes;
		std::uint64_t cache_clock;
	};

	extern java_chunk_c java_chunk;
}

//=====================================================================================
