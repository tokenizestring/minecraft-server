
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_chunk_c bedrock_chunk;

	void bedrock_chunk_c::build()
	{
		section.clear();
		write_section(section);

		spawn_area.clear();
		functions::write_u8(spawn_area, raknet_game_packet);
		functions::write_u8(spawn_area, bedrock_compression_none);

		for (auto x{ -view_radius }; x <= view_radius; ++x)
		{
			for (auto z{ -view_radius }; z <= view_radius; ++z)
			{
				packet.clear();
				write_chunk(packet, x, z);
				functions::write_varint(spawn_area, static_cast<std::int32_t>(packet.size()));
				functions::write_bytes(spawn_area, packet.data(), packet.size());
			}
		}

		utils::log.write(color_play, "[play](build) 'bedrock %s' %.1f kb", bedrock_version, static_cast<std::double_t>(spawn_area.size()) / 1024.0);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chunk_c::write_chunk(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z)
	{
		std::vector<std::uint8_t> payload{};

		for (auto index{ 0u }; index < bedrock_section_count; ++index)
		{
			functions::write_u8(payload, 1u);
			functions::write_zigzag32(payload, bedrock_biome_plains);
		}

		functions::write_u8(payload, 0u);

		functions::write_varint(out, static_cast<std::int32_t>(bedrock_level_chunk));
		functions::write_zigzag32(out, x);
		functions::write_zigzag32(out, z);
		functions::write_zigzag32(out, 0);
		functions::write_varint(out, 0);
		functions::write_zigzag32(out, bedrock_request_no_limit);
		functions::write_varint(out, static_cast<std::int32_t>(bedrock_section_count));
		functions::write_bool(out, false);
		functions::write_varint(out, 0);
		functions::write_varint(out, static_cast<std::int32_t>(payload.size()));
		functions::write_bytes(out, payload.data(), payload.size());
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chunk_c::write_section(std::vector<std::uint8_t>& out)
	{
		functions::write_u8(out, static_cast<std::uint8_t>(bedrock_subchunk_version));
		functions::write_u8(out, 1u);
		functions::write_u8(out, static_cast<std::uint8_t>(spawn_block_y >> 4));
		functions::write_u8(out, static_cast<std::uint8_t>((bedrock_storage_bits << 1u) | 1u));

		for (auto word{ 0u }; word < section_volume / 32u; ++word)
		{
			auto packed{ 0u };

			for (auto slot{ 0u }; slot < 32u; ++slot)
			{
				if (is_platform_bedrock_index(word * 32u + slot))
				{
					packed |= 1u << slot;
				}
			}

			functions::write_u32_le(out, packed);
		}

		functions::write_zigzag32(out, 2);
		functions::write_zigzag32(out, bedrock_block_air);
		functions::write_zigzag32(out, bedrock_block_grass);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chunk_c::handle_subchunk_request(structures::session_s& session, structures::reader_s& in)
	{
		auto dimension{ functions::read_zigzag32(in) };
		auto count{ static_cast<std::uint32_t>(functions::read_varint(in)) };
		auto offsets{ in.cursor };

		if (in.good() && count <= bedrock_max_subchunk_requests && in.remaining() >= static_cast<std::size_t>(count) * 3u)
		{
			in.cursor += static_cast<std::size_t>(count) * 3u;
		}

		else
		{
			in.valid = false;
		}

		auto center_x{ static_cast<std::int32_t>(functions::read_u32_le(in)) };
		auto center_y{ static_cast<std::int32_t>(functions::read_u32_le(in)) };
		auto center_z{ static_cast<std::int32_t>(functions::read_u32_le(in)) };

		if (in.good() && count <= bedrock_max_subchunk_requests)
		{
			body.clear();
			functions::write_bool(body, false);
			functions::write_zigzag32(body, dimension);
			functions::write_u32_le(body, static_cast<std::uint32_t>(center_x));
			functions::write_u32_le(body, static_cast<std::uint32_t>(center_y));
			functions::write_u32_le(body, static_cast<std::uint32_t>(center_z));
			functions::write_varint(body, static_cast<std::int32_t>(count));

			for (auto index{ 0u }; index < count; ++index)
			{
				auto offset{ in.data + offsets + index * 3u };
				auto x{ center_x + static_cast<std::int8_t>(offset[0]) };
				auto y{ center_y + static_cast<std::int8_t>(offset[1]) };
				auto z{ center_z + static_cast<std::int8_t>(offset[2]) };

				functions::write_bytes(body, offset, 3u);

				if (x == (spawn_block_x >> 4) && z == (spawn_block_z >> 4) && y == (spawn_block_y >> 4))
				{
					functions::write_u8(body, bedrock_subchunk_success);
					functions::write_u8(body, 1u);
					functions::write_varint(body, static_cast<std::int32_t>(section.size()));
					functions::write_bytes(body, section.data(), section.size());
					functions::write_bytes(body, bedrock_heightmaps_none, sizeof(bedrock_heightmaps_none));
				}

				else if (std::abs(x - (spawn_block_x >> 4)) <= view_radius && std::abs(z - (spawn_block_z >> 4)) <= view_radius && y >= bedrock_min_section && y < bedrock_min_section + static_cast<std::int32_t>(bedrock_section_count))
				{
					functions::write_u8(body, bedrock_subchunk_all_air);
					functions::write_u8(body, 0u);
					functions::write_bytes(body, bedrock_heightmaps_air, sizeof(bedrock_heightmaps_air));
				}

				else
				{
					functions::write_u8(body, bedrock_subchunk_missing);
					functions::write_u8(body, 0u);
					functions::write_bytes(body, bedrock_heightmaps_none, sizeof(bedrock_heightmaps_none));
				}
			}

			bedrock_packet.send(session, bedrock_subchunk, body);
		}
	}
	/*
	//=====================================================================================
	*/
	const std::vector<std::uint8_t>& bedrock_chunk_c::get_spawn_area() const
	{
		return spawn_area;
	}
}

//=====================================================================================
