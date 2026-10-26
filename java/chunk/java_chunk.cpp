
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_chunk_c java_chunk;

	void java_chunk_c::build()
	{
		auto radius{ static_cast<std::int32_t>(config.view_distance) };

		for (auto x{ -radius }; x <= radius; ++x)
		{
			for (auto z{ -radius }; z <= radius; ++z)
			{
				offsets.emplace_back(x, z);
			}
		}

		std::sort(offsets.begin(), offsets.end(), [](const auto& a, const auto& b) { return a.first * a.first + a.second * a.second < b.first * b.first + b.second * b.second; });

		packet.clear();
		write_chunk_1_8(packet, spawn_block_x >> 4, spawn_block_z >> 4);
		functions::write_frame(world_1_8, packet);

		utils::log.write(color_play, "[play](build) java view distance %u, %zu chunks per player, chunk cache %u mb, '1.8.x' %.1f kb", config.view_distance, offsets.size(), config.chunk_cache_mb, static_cast<std::double_t>(world_1_8.size()) / 1024.0);
	}
	/*
	//=====================================================================================
	*/
	const std::vector<std::uint8_t>& java_chunk_c::get_1_8() const
	{
		return world_1_8;
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_view(structures::client_s& client)
	{
		client.view_x = get_chunk(client.player.x);
		client.view_z = get_chunk(client.player.z);
		send_center(client, client.view_x, client.view_z);

		for (const auto& [x, z] : offsets)
		{
			send_column(client, client.view_x + x, client.view_z + z);
		}

		trim_cache();
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
					send_column(client, chunk_x + x, chunk_z + z);
				}

				if (std::abs(client.view_x + x - chunk_x) > radius || std::abs(client.view_z + z - chunk_z) > radius)
				{
					send_forget(client, client.view_x + x, client.view_z + z);
				}
			}

			client.view_x = chunk_x;
			client.view_z = chunk_z;
			trim_cache();
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_center(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		packet.clear();
		functions::write_varint(packet, client.layout.play_out_center_chunk);
		functions::write_varint(packet, chunk_x);
		functions::write_varint(packet, chunk_z);
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_column(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		network.send_held(client, get_column(client.family, chunk_x, chunk_z));
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::send_forget(structures::client_s& client, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		packet.clear();
		functions::write_varint(packet, client.layout.play_out_forget_chunk);
		functions::write_u64(packet, (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_z)) << 32u) | static_cast<std::uint32_t>(chunk_x));
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<std::uint8_t>> java_chunk_c::get_column(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto key{ (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_x)) << 32u) | static_cast<std::uint32_t>(chunk_z) };
		auto& entry{ cache[family][key] };
		cache_clock += 1u;
		entry.last_use = cache_clock;
		entry.packet = entry.packet ? entry.packet : build_column(family, chunk_x, chunk_z);

		return entry.packet;
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<std::uint8_t>> java_chunk_c::build_column(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		packet.clear();
		write_column(packet, family, chunk_x, chunk_z);

		auto framed{ std::make_shared<std::vector<std::uint8_t>>() };
		functions::write_frame(*framed, packet);
		framed->shrink_to_fit();
		cache_bytes += framed->size();

		return framed;
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

			for (const auto& family : cache)
			{
				for (const auto& [key, entry] : family)
				{
					ages.push_back(entry.last_use);
				}
			}

			auto keep{ ages.size() / 2u };
			std::nth_element(ages.begin(), ages.begin() + static_cast<std::ptrdiff_t>(ages.size() - keep), ages.end());
			auto cutoff{ ages[ages.size() - keep] };

			for (auto& family : cache)
			{
				for (auto item{ family.begin() }; item != family.end();)
				{
					if (item->second.last_use < cutoff)
					{
						cache_bytes -= item->second.packet->size();
						item = family.erase(item);
					}

					else
					{
						++item;
					}
				}
			}

			utils::log.write(color_play, "[play](cache) chunk cache trimmed to %.1f mb", static_cast<std::double_t>(cache_bytes) / 1048576.0);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::write_column(std::vector<std::uint8_t>& out, structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		const auto& layout{ functions::get_layout(family) };

		functions::write_varint(out, layout.play_out_chunk);
		functions::write_i32(out, chunk_x);
		functions::write_i32(out, chunk_z);
		write_heightmaps(out, chunk_x, chunk_z);

		std::vector<std::uint8_t> sections{};

		for (auto index{ 0u }; index < modern_section_count; ++index)
		{
			write_section(sections, family, terrain.get_section(chunk_x, modern_min_section + static_cast<std::int32_t>(index), chunk_z));
		}

		functions::write_varint(out, static_cast<std::int32_t>(sections.size()));
		functions::write_bytes(out, sections.data(), sections.size());
		functions::write_varint(out, 0);

		functions::write_bitset(out, 0ui64, layout.byte_bitsets);
		functions::write_bitset(out, 0ui64, layout.byte_bitsets);
		functions::write_bitset(out, 0ui64, layout.byte_bitsets);
		functions::write_bitset(out, 0ui64, layout.byte_bitsets);
		functions::write_varint(out, 0);
		functions::write_varint(out, 0);
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::write_heightmaps(std::vector<std::uint8_t>& out, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		std::array<std::uint32_t, 256u> heights{};

		for (auto index{ static_cast<std::int32_t>(modern_section_count) - 1 }; index >= 0; --index)
		{
			if (const auto* section{ terrain.get_section(chunk_x, modern_min_section + index, chunk_z) }; section)
			{
				for (auto column{ 0u }; column < 256u; ++column)
				{
					for (auto y{ 15 }; y >= 0 && heights[column] == 0u; --y)
					{
						if (terrain.is_solid(*section, static_cast<std::uint32_t>(y) * 256u + column))
						{
							heights[column] = static_cast<std::uint32_t>(index * 16 + y + 1);
						}
					}
				}
			}
		}

		functions::write_varint(out, static_cast<std::int32_t>(std::size(modern_heightmap_types)));

		for (auto type : modern_heightmap_types)
		{
			functions::write_varint(out, type);
			functions::write_varint(out, static_cast<std::int32_t>(modern_heightmap_longs));

			for (auto word{ 0u }; word < modern_heightmap_longs; ++word)
			{
				auto packed{ 0ui64 };

				for (auto slot{ 0u }; slot < modern_heightmap_per_long && word * modern_heightmap_per_long + slot < 256u; ++slot)
				{
					packed |= static_cast<std::uint64_t>(heights[word * modern_heightmap_per_long + slot]) << (slot * modern_heightmap_bits);
				}

				functions::write_u64(out, packed);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::write_section(std::vector<std::uint8_t>& out, structures::family_e family, const structures::terrain_section_s* section)
	{
		auto java_id{ [family](std::uint32_t state) { return static_cast<std::int32_t>(family == structures::family_26_2 ? terrain.get_state(state).java_26_2 : terrain.get_state(state).java_26_3); } };

		if (section && section->palette_size == 1u)
		{
			functions::write_u16(out, section->non_air);
			functions::write_u16(out, section->fluid);
			functions::write_u8(out, 0u);
			functions::write_varint(out, java_id(terrain.get_palette(*section)[0]));
		}

		else if (section && section->bits <= 8u)
		{
			functions::write_u16(out, section->non_air);
			functions::write_u16(out, section->fluid);
			functions::write_u8(out, section->bits);
			functions::write_varint(out, section->palette_size);

			for (auto index{ 0u }; index < section->palette_size; ++index)
			{
				functions::write_varint(out, java_id(terrain.get_palette(*section)[index]));
			}

			for (auto word{ 0u }; word < (section_volume + 64u / section->bits - 1u) / (64u / section->bits); ++word)
			{
				functions::write_u64(out, terrain.get_longs(*section)[word]);
			}
		}

		else if (section)
		{
			auto bits{ functions::get_layout(family).direct_bits };
			auto per_long{ 64u / bits };

			functions::write_u16(out, section->non_air);
			functions::write_u16(out, section->fluid);
			functions::write_u8(out, static_cast<std::uint8_t>(bits));

			for (auto word{ 0u }; word < (section_volume + per_long - 1u) / per_long; ++word)
			{
				auto packed{ 0ui64 };

				for (auto slot{ 0u }; slot < per_long && word * per_long + slot < section_volume; ++slot)
				{
					packed |= static_cast<std::uint64_t>(java_id(terrain.get_block(*section, word * per_long + slot))) << (slot * bits);
				}

				functions::write_u64(out, packed);
			}
		}

		else
		{
			functions::write_u16(out, 0u);
			functions::write_u16(out, 0u);
			functions::write_u8(out, 0u);
			functions::write_varint(out, 0);
		}

		functions::write_u8(out, 0u);
		functions::write_varint(out, static_cast<std::int32_t>(java_registry.get(family).plains));
	}
	/*
	//=====================================================================================
	*/
	void java_chunk_c::write_chunk_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z)
	{
		functions::write_varint(out, v1_8_play_out_chunk);
		functions::write_i32(out, x);
		functions::write_i32(out, z);
		functions::write_bool(out, true);
		functions::write_u16(out, static_cast<std::uint16_t>(1u << (spawn_block_y >> 4)));
		functions::write_varint(out, static_cast<std::int32_t>(section_volume * 2u + light_array_size * 2u + biome_column_count));

		for (auto index{ 0u }; index < section_volume; ++index)
		{
			functions::write_u8(out, is_platform_index(index) ? static_cast<std::uint8_t>(v1_8_grass_block & 0xFFu) : 0u);
			functions::write_u8(out, is_platform_index(index) ? static_cast<std::uint8_t>(v1_8_grass_block >> 8u) : 0u);
		}

		out.insert(out.end(), light_array_size, 0x00u);
		out.insert(out.end(), light_array_size, 0xFFu);
		out.insert(out.end(), biome_column_count, v1_8_plains);
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
