
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_chunk_c java_chunk;

	void java_chunk_c::build()
	{
		auto radius{ static_cast<std::int32_t>(config.view_distance) };
		auto threads{ config.chunk_threads ? config.chunk_threads : std::max(1u, std::thread::hardware_concurrency() - 1u) };

		for (auto x{ -radius }; x <= radius; ++x)
		{
			for (auto z{ -radius }; z <= radius; ++z)
			{
				offsets.emplace_back(x, z);
			}
		}

		std::sort(offsets.begin(), offsets.end(), [](const auto& a, const auto& b) { return a.first * a.first + a.second * a.second < b.first * b.first + b.second * b.second; });
		running = true;

		for (auto index{ 0u }; index < threads; ++index)
		{
			auto* builder{ builders.emplace_back(std::make_unique<java_builder_c>()).get() };
			workers.emplace_back([this, builder]() { work(*builder); });
		}

		utils::log.write(color_play, "[play](build) java view distance %u, %zu chunks per player, %u chunk threads, %u chunks per player per tick, chunk cache %u mb", config.view_distance, offsets.size(), threads, config.chunk_budget, config.chunk_cache_mb);
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::shutdown()
	{
		running = false;
		wake.notify_all();

		for (auto& worker : workers)
		{
			worker.join();
		}

		workers.clear();
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::tick()
	{
		collect();

		for (auto player : world.get_players())
		{
			if (player->edition == structures::edition_java)
			{
				feed(*static_cast<structures::client_s*>(player->link));
			}
		}

		trim_cache();
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_view(structures::client_s& client)
	{
		client.view_x = get_chunk(client.player.x);
		client.view_z = get_chunk(client.player.z);
		client.wanted.clear();
		send_center(client, client.view_x, client.view_z);

		for (const auto& [x, z] : offsets)
		{
			client.wanted.push_back(get_key(client.view_x + x, client.view_z + z));
		}

		feed(client);
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::update_view(structures::client_s& client)
	{
		auto chunk_x{ get_chunk(client.player.x) };
		auto chunk_z{ get_chunk(client.player.z) };
		auto radius{ static_cast<std::int32_t>(config.view_distance) };

		if (chunk_x != client.view_x || chunk_z != client.view_z)
		{
			send_center(client, chunk_x, chunk_z);

			for (const auto& [x, z] : offsets)
			{
				if (std::abs(chunk_x + x - client.view_x) > radius || std::abs(chunk_z + z - client.view_z) > radius)
				{
					client.wanted.push_back(get_key(chunk_x + x, chunk_z + z));
				}

				if (std::abs(client.view_x + x - chunk_x) > radius || std::abs(client.view_z + z - chunk_z) > radius)
				{
					send_forget(client, client.view_x + x, client.view_z + z);
				}
			}

			client.view_x = chunk_x;
			client.view_z = chunk_z;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::clear_view(structures::client_s& client)
	{
		for (const auto& [x, z] : offsets)
		{
			send_forget(client, client.view_x + x, client.view_z + z);
		}

		client.wanted.clear();
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::reset_world(structures::world_e target)
	{
		epochs[target] += 1u;

		for (auto& family : cache[target])
		{
			for (const auto& [key, entry] : family)
			{
				cache_bytes -= entry.packet ? entry.packet->size() : 0u;
			}

			family.clear();
		}

		for (auto& family : pending[target])
		{
			family.clear();
		}

		std::lock_guard lock{ jobs_lock };
		std::erase_if(jobs, [target](const auto& job) { return job.world == target; });
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_center(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		if (functions::is_modern(client.family))
		{
			packet.clear();
			functions::write_varint(packet, client.layout.play_out_center_chunk);
			functions::write_varint(packet, chunk_x);
			functions::write_varint(packet, chunk_z);
			network.send(client, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_forget(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_forget_chunk);
			functions::write_u64(packet, (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_z)) << 32u) | static_cast<std::uint32_t>(chunk_x));
		}

		else
		{
			java_builder_c::write_empty_1_8(packet, chunk_x, chunk_z);
		}

		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::feed(structures::client_s& client)
	{
		auto radius{ static_cast<std::int32_t>(config.view_distance) };
		auto budget{ config.chunk_budget };
		auto kept{ std::size_t{ 0u } };

		for (auto key : client.wanted)
		{
			auto chunk_x{ static_cast<std::int32_t>(key >> 32u) };
			auto chunk_z{ static_cast<std::int32_t>(key & 0xFFFFFFFFui64) };
			auto inside{ std::abs(chunk_x - client.view_x) <= radius && std::abs(chunk_z - client.view_z) <= radius };
			auto cached{ inside && request(client.family, client.player.world, chunk_x, chunk_z) };

			if (cached && budget)
			{
				network.send_held(client, cache[client.player.world][client.family][key].packet);
				budget -= 1u;
			}

			else if (inside)
			{
				client.wanted[kept] = key;
				kept += 1u;
			}
		}

		client.wanted.resize(kept);
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::collect()
	{
		ready.clear();

		{
			std::lock_guard lock{ done_lock };
			ready.swap(done);
		}

		for (auto& job : ready)
		{
			if (job.epoch == epochs[job.world])
			{
				auto key{ get_key(job.x, job.z) };
				auto& entry{ cache[job.world][job.family][key] };
				cache_bytes += job.packet->size() - (entry.packet ? entry.packet->size() : 0u);
				cache_clock += 1u;
				entry.packet = std::move(job.packet);
				entry.last_use = cache_clock;
				pending[job.world][job.family].erase(key);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	bool java_chunk_c::request(structures::family_e family, structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto key{ get_key(chunk_x, chunk_z) };
		auto found{ cache[target][family].find(key) };
		auto cached{ found != cache[target][family].end() && found->second.packet };

		if (cached)
		{
			cache_clock += 1u;
			found->second.last_use = cache_clock;
		}

		else if (pending[target][family].insert(key).second)
		{
			{
				std::lock_guard lock{ jobs_lock };
				jobs.push_back(structures::chunk_job_s{ target, family, chunk_x, chunk_z, config.smp_seed, epochs[target], {} });
			}

			wake.notify_one();
		}

		return cached;
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::work(java_builder_c& builder)
	{
		while (running)
		{
			structures::chunk_job_s job{};
			auto taken{ false };

			{
				std::unique_lock lock{ jobs_lock };
				wake.wait(lock, [this]() { return !running || jobs.size(); });

				if (running && jobs.size())
				{
					job = jobs.front();
					jobs.pop_front();
					taken = true;
				}
			}

			if (taken)
			{
				job.packet = builder.build(job);
				std::lock_guard lock{ done_lock };
				done.push_back(std::move(job));
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::trim_cache()
	{
		auto budget{ static_cast<std::size_t>(config.chunk_cache_mb) * 1048576u };

		if (cache_bytes > budget)
		{
			std::vector<std::uint64_t> ages{};

			for (const auto& worlds : cache)
			{
				for (const auto& family : worlds)
				{
					for (const auto& [key, entry] : family)
					{
						ages.push_back(entry.last_use);
					}
				}
			}

			auto keep{ ages.size() / 2u };
			std::nth_element(ages.begin(), ages.begin() + static_cast<std::ptrdiff_t>(ages.size() - keep), ages.end());
			auto cutoff{ ages[ages.size() - keep] };

			for (auto& worlds : cache)
			{
				for (auto& family : worlds)
				{
					for (auto item{ family.begin() }; item != family.end();)
					{
						if (item->second.last_use < cutoff)
						{
							cache_bytes -= item->second.packet ? item->second.packet->size() : 0u;
							item = family.erase(item);
						}

						else
						{
							++item;
						}
					}
				}
			}

			utils::log.write(color_play, "[play](cache) chunk cache trimmed to %.1f mb", static_cast<std::double_t>(cache_bytes) / 1048576.0);
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t java_chunk_c::get_key(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_x)) << 32u) | static_cast<std::uint32_t>(chunk_z);
	}
	/*
	//=====================================================================================
	*/
	std::int32_t java_chunk_c::get_chunk(std::double_t position)
	{
		return static_cast<std::int32_t>(std::floor(position / 16.0));
	}
}

//=====================================================================================
