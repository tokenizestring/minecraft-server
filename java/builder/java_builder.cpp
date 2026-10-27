
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	std::shared_ptr<const std::vector<std::uint8_t>> java_builder_c::build(const structures::chunk_job_s& job)
	{
		packet.clear();
		overrides = job.edits.get();

		if (job.world == structures::world_smp && seed != job.seed)
		{
			generator.set_seed(job.seed);
			seed = job.seed;
		}

		if (job.world == structures::world_smp && functions::is_modern(job.family))
		{
			write_generated(job.family, job.x, job.z);
		}

		else if (job.world == structures::world_smp)
		{
			write_generated_1_8(job.x, job.z);
		}

		else if (functions::is_modern(job.family))
		{
			write_terrain(job.family, job.x, job.z);
		}

		else if (job.x == (spawn_block_x >> 4) && job.z == (spawn_block_z >> 4))
		{
			write_platform_1_8(packet, job.x, job.z);
		}

		else
		{
			write_empty_1_8(packet, job.x, job.z);
		}

		frame();
		return std::make_shared<std::vector<std::uint8_t>>(framed.begin(), framed.end());
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_empty_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z)
	{
		functions::write_varint(out, v1_8_play_out_chunk);
		functions::write_i32(out, x);
		functions::write_i32(out, z);
		functions::write_bool(out, true);
		functions::write_u16(out, 0u);
		functions::write_varint(out, 0);
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_generated(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		const auto& layout{ functions::get_layout(family) };
		std::array<std::uint32_t, 256u> heights{};
		auto highest{ world_min_y - 1 };

		generator.generate(chunk_x, chunk_z);
		place_edits();

		for (auto column{ 0u }; column < 256u; ++column)
		{
			auto top{ generator.get_top(column) };
			heights[column] = static_cast<std::uint32_t>(top - world_min_y + 1);
			highest = std::max(highest, top);
		}

		functions::write_varint(packet, layout.play_out_chunk);
		functions::write_i32(packet, chunk_x);
		functions::write_i32(packet, chunk_z);
		write_heightmaps(packet, heights);

		sections.clear();

		for (auto index{ 0u }; index < modern_section_count; ++index)
		{
			if (fill_kinds(index))
			{
				write_placed_section(sections, family);
			}

			else
			{
				write_generated_section(sections, family, generator.get_blocks() + static_cast<std::size_t>(index) * section_volume);
			}
		}

		functions::write_varint(packet, static_cast<std::int32_t>(sections.size()));
		functions::write_bytes(packet, sections.data(), sections.size());
		functions::write_varint(packet, 0);
		write_light(layout, light.compute_generated(generator.get_blocks(), highest));
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_generated_section(std::vector<std::uint8_t>& out, structures::family_e family, const std::uint8_t* column) const
	{
		std::array<std::uint8_t, 256u> slots{};
		std::array<std::uint8_t, 256u> palette{};
		auto count{ 0u };
		auto non_air{ 0u };
		auto fluid{ 0u };
		slots.fill(0xFFu);

		for (auto index{ 0u }; index < section_volume; ++index)
		{
			auto block{ column[index] };

			if (slots[block] == 0xFFu)
			{
				slots[block] = static_cast<std::uint8_t>(count);
				palette[count] = block;
				count += 1u;
			}

			non_air += block != structures::block_air ? 1u : 0u;
			fluid += blocks.is_fluid(block) ? 1u : 0u;
		}

		functions::write_u16(out, static_cast<std::uint16_t>(non_air));
		functions::write_u16(out, static_cast<std::uint16_t>(fluid));

		if (count == 1u)
		{
			functions::write_u8(out, 0u);
			functions::write_varint(out, static_cast<std::int32_t>(blocks.get_java(family, palette[0])));
		}

		else
		{
			auto bits{ std::max(4u, static_cast<std::uint32_t>(std::bit_width(count - 1u))) };
			auto per_long{ 64u / bits };

			functions::write_u8(out, static_cast<std::uint8_t>(bits));
			functions::write_varint(out, static_cast<std::int32_t>(count));

			for (auto index{ 0u }; index < count; ++index)
			{
				functions::write_varint(out, static_cast<std::int32_t>(blocks.get_java(family, palette[index])));
			}

			for (auto word{ 0u }; word < (section_volume + per_long - 1u) / per_long; ++word)
			{
				auto packed{ 0ui64 };

				for (auto slot{ 0u }; slot < per_long && word * per_long + slot < section_volume; ++slot)
				{
					packed |= static_cast<std::uint64_t>(slots[column[word * per_long + slot]]) << (slot * bits);
				}

				functions::write_u64(out, packed);
			}
		}

		functions::write_u8(out, 0u);
		functions::write_varint(out, static_cast<std::int32_t>(java_registry.get(family).plains));
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_placed_section(std::vector<std::uint8_t>& out, structures::family_e family)
	{
		auto non_air{ 0u };
		auto fluid{ 0u };
		palette.clear();

		for (auto index{ 0u }; index < section_volume; ++index)
		{
			auto kind{ kinds[index] };
			auto found{ std::find(palette.begin(), palette.end(), kind) };

			if (found == palette.end())
			{
				palette.push_back(kind);
				found = palette.end() - 1;
			}

			slots[index] = static_cast<std::uint16_t>(found - palette.begin());
			non_air += kind != structures::block_air ? 1u : 0u;
			fluid += kind < placed_kind_base && blocks.is_fluid(static_cast<std::uint8_t>(kind)) ? 1u : 0u;
		}

		auto count{ static_cast<std::uint32_t>(palette.size()) };
		auto bits{ std::max(4u, static_cast<std::uint32_t>(std::bit_width(count - 1u))) };
		auto indirect{ bits <= 8u };
		bits = indirect ? bits : functions::get_layout(family).direct_bits;
		auto per_long{ 64u / bits };

		functions::write_u16(out, static_cast<std::uint16_t>(non_air));
		functions::write_u16(out, static_cast<std::uint16_t>(fluid));

		if (count == 1u)
		{
			functions::write_u8(out, 0u);
			functions::write_varint(out, static_cast<std::int32_t>(blocks.get_state(family, palette[0])));
		}

		else
		{
			functions::write_u8(out, static_cast<std::uint8_t>(bits));

			if (indirect)
			{
				functions::write_varint(out, static_cast<std::int32_t>(count));

				for (auto kind : palette)
				{
					functions::write_varint(out, static_cast<std::int32_t>(blocks.get_state(family, kind)));
				}
			}

			for (auto word{ 0u }; word < (section_volume + per_long - 1u) / per_long; ++word)
			{
				auto packed{ 0ui64 };

				for (auto slot{ 0u }; slot < per_long && word * per_long + slot < section_volume; ++slot)
				{
					auto value{ indirect ? static_cast<std::uint32_t>(slots[word * per_long + slot]) : blocks.get_state(family, kinds[word * per_long + slot]) };
					packed |= static_cast<std::uint64_t>(value) << (slot * bits);
				}

				functions::write_u64(out, packed);
			}
		}

		functions::write_u8(out, 0u);
		functions::write_varint(out, static_cast<std::int32_t>(java_registry.get(family).plains));
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::place_edits()
	{
		if (overrides)
		{
			generator.apply(*overrides);
		}
	}
	/*
	//=====================================================================================
	*/
	bool java_builder_c::fill_kinds(std::uint32_t section)
	{
		auto first{ section * section_volume };
		auto placed{ overrides && std::any_of(overrides->begin(), overrides->end(), [first](const auto& edit) { return edit.index - first < section_volume && edit.kind >= placed_kind_base; }) };

		if (placed)
		{
			const auto* column{ generator.get_blocks() + first };
			std::copy(column, column + section_volume, kinds.begin());

			for (const auto& edit : *overrides)
			{
				if (edit.index - first < section_volume)
				{
					kinds[edit.index - first] = edit.kind;
				}
			}
		}

		return placed;
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_generated_1_8(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		auto highest{ world_min_y - 1 };
		auto mask{ 0u };

		generator.generate(chunk_x, chunk_z);
		place_edits();

		for (auto column{ 0u }; column < 256u; ++column)
		{
			highest = std::max(highest, generator.get_top(column));
		}

		auto lit{ light.compute_generated(generator.get_blocks(), highest) };

		for (auto section{ 0u }; section < v1_8_section_count; ++section)
		{
			const auto* column{ generator.get_blocks() + static_cast<std::size_t>(section + v1_8_section_offset) * section_volume };

			if (std::any_of(column, column + section_volume, [](auto block) { return block != structures::block_air; }))
			{
				mask |= 1u << section;
			}
		}

		sections.clear();

		for (auto section{ 0u }; section < v1_8_section_count; ++section)
		{
			if ((mask >> section) & 1u)
			{
				const auto* column{ generator.get_blocks() + static_cast<std::size_t>(section + v1_8_section_offset) * section_volume };
				auto placed{ fill_kinds(section + v1_8_section_offset) };

				for (auto index{ 0u }; index < section_volume; ++index)
				{
					auto id{ placed ? blocks.get_state(structures::family_1_8, kinds[index]) : blocks.get_java(structures::family_1_8, column[index]) };
					sections.push_back(static_cast<std::uint8_t>(id));
					sections.push_back(static_cast<std::uint8_t>(id >> 8u));
				}
			}
		}

		sections.insert(sections.end(), static_cast<std::size_t>(std::popcount(mask)) * light_array_size, 0x00u);

		for (auto section{ 0u }; section < v1_8_section_count; ++section)
		{
			if (((mask >> section) & 1u) && section + v1_8_section_offset < lit)
			{
				functions::write_bytes(sections, light.get_section(section + v1_8_section_offset), light_array_size);
			}

			else if ((mask >> section) & 1u)
			{
				sections.insert(sections.end(), light_array_size, 0xFFu);
			}
		}

		sections.insert(sections.end(), biome_column_count, v1_8_plains);

		functions::write_varint(packet, v1_8_play_out_chunk);
		functions::write_i32(packet, chunk_x);
		functions::write_i32(packet, chunk_z);
		functions::write_bool(packet, true);
		functions::write_u16(packet, static_cast<std::uint16_t>(mask));
		functions::write_varint(packet, static_cast<std::int32_t>(sections.size()));
		functions::write_bytes(packet, sections.data(), sections.size());
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_terrain(structures::family_e family, std::int32_t chunk_x, std::int32_t chunk_z)
	{
		const auto& layout{ functions::get_layout(family) };
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

		functions::write_varint(packet, layout.play_out_chunk);
		functions::write_i32(packet, chunk_x);
		functions::write_i32(packet, chunk_z);
		write_heightmaps(packet, heights);

		sections.clear();

		for (auto index{ 0u }; index < modern_section_count; ++index)
		{
			write_terrain_section(sections, family, terrain.get_section(chunk_x, modern_min_section + static_cast<std::int32_t>(index), chunk_z));
		}

		functions::write_varint(packet, static_cast<std::int32_t>(sections.size()));
		functions::write_bytes(packet, sections.data(), sections.size());
		functions::write_varint(packet, 0);
		write_light(layout, light.compute_sky(chunk_x, chunk_z));
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_light(const structures::layout_s& layout, std::uint32_t lit)
	{
		functions::write_bitset(packet, ((1ui64 << lit) - 1ui64) << 1u, layout.byte_bitsets);
		functions::write_bitset(packet, 0ui64, layout.byte_bitsets);
		functions::write_bitset(packet, 0ui64, layout.byte_bitsets);
		functions::write_bitset(packet, 0ui64, layout.byte_bitsets);
		functions::write_varint(packet, static_cast<std::int32_t>(lit));

		for (auto section{ 0u }; section < lit; ++section)
		{
			functions::write_varint(packet, static_cast<std::int32_t>(light_array_size));
			functions::write_bytes(packet, light.get_section(section), light_array_size);
		}

		functions::write_varint(packet, 0);
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::frame()
	{
		framed.clear();

		if (packet.size() >= java_compression_threshold)
		{
			compressed.clear();
			deflater.compress(packet.data(), packet.size(), compressed, true);
			functions::write_varint(framed, static_cast<std::int32_t>(functions::get_varint_size(static_cast<std::int32_t>(packet.size())) + compressed.size()));
			functions::write_varint(framed, static_cast<std::int32_t>(packet.size()));
			functions::write_bytes(framed, compressed.data(), compressed.size());
		}

		else
		{
			functions::write_packet_frame(framed, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_builder_c::write_heightmaps(std::vector<std::uint8_t>& out, const std::array<std::uint32_t, 256u>& heights)
	{
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
	void java_builder_c::write_terrain_section(std::vector<std::uint8_t>& out, structures::family_e family, const structures::terrain_section_s* section)
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
	void java_builder_c::write_platform_1_8(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z)
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
}

//=====================================================================================
