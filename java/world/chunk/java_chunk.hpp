
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_chunk_c
	{
	public:

		void build();
		void shutdown();
		void tick();
		void send_view(structures::client_s& client);
		void update_view(structures::client_s& client);
		void clear_view(structures::client_s& client);
		void reset_world(structures::world_e target);
		void invalidate(structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z);
		std::size_t get_cached() const;

	private:

		void send_center(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void send_forget(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z);
		void feed(structures::client_s& client);
		void collect();
		void trim_cache();
		void work(java_builder_c& builder);
		bool request(structures::family_e family, structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z);

		static std::uint64_t get_key(std::int32_t chunk_x, std::int32_t chunk_z);
		static std::int32_t get_chunk(std::double_t position);

		std::array<std::array<std::unordered_map<std::uint64_t, structures::chunk_cache_s>, structures::family_count>, structures::world_count> cache;
		std::array<std::array<std::unordered_set<std::uint64_t>, structures::family_count>, structures::world_count> pending;
		std::array<std::uint32_t, structures::world_count> epochs;
		std::vector<std::pair<std::int32_t, std::int32_t>> offsets;
		std::vector<std::unique_ptr<java_builder_c>> builders;
		std::vector<std::thread> workers;
		std::deque<structures::chunk_job_s> jobs;
		std::vector<structures::chunk_job_s> done;
		std::vector<structures::chunk_job_s> ready;
		std::mutex jobs_lock;
		std::mutex done_lock;
		std::condition_variable wake;
		std::atomic<bool> running;
		std::vector<std::uint8_t> packet;
		std::size_t cache_bytes;
		std::uint64_t cache_clock;
	};

	extern java_chunk_c java_chunk;
}

//=====================================================================================
