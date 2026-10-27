
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	terrain_c terrain;

	void terrain_c::load()
	{
		std::vector<std::uint8_t> file{};

		if (auto handle{ std::fopen(functions::get_local_path(terrain_file_name).c_str(), "rb") }; handle)
		{
			std::fseek(handle, 0, SEEK_END);
			file.resize(static_cast<std::size_t>(std::ftell(handle)));
			std::fseek(handle, 0, SEEK_SET);
			file.resize(std::fread(file.data(), 1u, file.size(), handle));
			std::fclose(handle);
		}

		structures::reader_s in{ file.data(), file.size(), 0u, true };
		std::array<std::int32_t, 3u> spawn{};
		auto magic{ functions::read_u32_le(in) };
		auto version{ functions::read_u32_le(in) };
		functions::read_raw(in, spawn.data(), sizeof(spawn));
		functions::read_raw(in, &min_chunk_x, sizeof(min_chunk_x));
		functions::read_raw(in, &min_chunk_z, sizeof(min_chunk_z));
		functions::read_raw(in, &chunks_x, sizeof(chunks_x));
		functions::read_raw(in, &chunks_z, sizeof(chunks_z));
		functions::read_raw(in, &min_section_y, sizeof(min_section_y));
		functions::read_raw(in, &sections_y, sizeof(sections_y));

		auto state_count{ functions::read_u32_le(in) };
		states.resize(in.good() && state_count <= in.remaining() / 11u ? state_count : 0u);

		for (auto& state : states)
		{
			std::uint16_t length{};
			functions::read_raw(in, &length, sizeof(length));
			in.cursor += length;
			functions::read_raw(in, &state.java_26_2, sizeof(state.java_26_2));
			functions::read_raw(in, &state.java_26_3, sizeof(state.java_26_3));
			state.flags = functions::read_u8(in);
		}

		auto section_count{ functions::read_u32_le(in) };
		sections.resize(in.good() && section_count <= in.remaining() / 7u ? section_count : 0u);

		for (auto& section : sections)
		{
			functions::read_raw(in, &section.palette_size, sizeof(section.palette_size));
			functions::read_raw(in, &section.non_air, sizeof(section.non_air));
			functions::read_raw(in, &section.fluid, sizeof(section.fluid));
			section.bits = functions::read_u8(in);
			section.palette = static_cast<std::uint32_t>(palettes.size());
			section.longs = static_cast<std::uint32_t>(longs.size());
			read_array(in, palettes, section.palette_size);
			read_array(in, longs, section.bits ? (section_volume + 64u / section.bits - 1u) / (64u / section.bits) : 0u);
		}

		columns.resize(in.good() && chunks_x > 0 && chunks_z > 0 && static_cast<std::size_t>(chunks_x) * static_cast<std::size_t>(chunks_z) <= in.remaining() / 4u ? static_cast<std::size_t>(chunks_x) * static_cast<std::size_t>(chunks_z) : 0u);

		for (auto& column : columns)
		{
			column.mask = functions::read_u32_le(in);
			column.refs = static_cast<std::uint32_t>(refs.size());
			read_array(in, refs, static_cast<std::size_t>(std::popcount(column.mask)));
		}

		auto valid{ in.good() && sections_y > 0 && sections_y <= 32 && magic == terrain_magic && version == terrain_version && columns.size() && std::all_of(refs.begin(), refs.end(), [this](auto ref) { return ref < sections.size(); }) && std::all_of(palettes.begin(), palettes.end(), [this](auto entry) { return entry < states.size(); }) && std::all_of(sections.begin(), sections.end(), [](const auto& section) { return (section.bits == 0u && section.palette_size == 1u) || (section.bits >= 4u && section.bits <= 16u && section.palette_size >= 2u && section.palette_size <= (1u << section.bits)); }) };

		if (valid)
		{
			utils::log.write(color_server, "[terrain](load) '%s' %d x %d chunks, %zu sections, %zu states, %.1f mb, spawn %d %d %d", terrain_file_name, chunks_x, chunks_z, sections.size(), states.size(), static_cast<std::double_t>(longs.size() * 8u + palettes.size() * 2u + refs.size() * 4u) / 1048576.0, spawn[0], spawn[1], spawn[2]);
		}

		else
		{
			sections.clear();
			columns.clear();
			utils::log.write(color_warn, "[terrain](missing) '%s' not loaded, java gets an empty world", terrain_file_name);
		}
	}
	/*
	//=====================================================================================
	*/
	const structures::terrain_section_s* terrain_c::get_section(std::int32_t chunk_x, std::int32_t section_y, std::int32_t chunk_z) const
	{
		const structures::terrain_section_s* found{ nullptr };
		auto x{ chunk_x - min_chunk_x };
		auto y{ section_y - min_section_y };
		auto z{ chunk_z - min_chunk_z };

		if (columns.size() && x >= 0 && x < chunks_x && z >= 0 && z < chunks_z && y >= 0 && y < sections_y)
		{
			if (const auto& column{ columns[static_cast<std::size_t>(z) * static_cast<std::size_t>(chunks_x) + static_cast<std::size_t>(x)] }; (column.mask >> y) & 1u)
			{
				found = &sections[refs[column.refs + static_cast<std::uint32_t>(std::popcount(column.mask & ((1u << y) - 1u)))]];
			}
		}

		return found;
	}
	/*
	//=====================================================================================
	*/
	const structures::terrain_state_s& terrain_c::get_state(std::uint32_t index) const
	{
		return states[index];
	}
	/*
	//=====================================================================================
	*/
	const std::uint16_t* terrain_c::get_palette(const structures::terrain_section_s& section) const
	{
		return palettes.data() + section.palette;
	}
	/*
	//=====================================================================================
	*/
	const std::uint64_t* terrain_c::get_longs(const structures::terrain_section_s& section) const
	{
		return longs.data() + section.longs;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t terrain_c::get_block(const structures::terrain_section_s& section, std::uint32_t index) const
	{
		auto local{ 0u };

		if (section.bits)
		{
			auto per_long{ 64u / section.bits };
			local = static_cast<std::uint32_t>((longs[section.longs + index / per_long] >> ((index % per_long) * section.bits)) & ((1ui64 << section.bits) - 1ui64));
		}

		return palettes[section.palette + std::min<std::uint32_t>(local, section.palette_size - 1u)];
	}
	/*
	//=====================================================================================
	*/
	bool terrain_c::is_solid(const structures::terrain_section_s& section, std::uint32_t index) const
	{
		return (states[get_block(section, index)].flags & terrain_flag_air) != terrain_flag_air;
	}
	/*
	//=====================================================================================
	*/
	bool terrain_c::is_solid_at(std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		const auto* section{ get_section(x >> 4, y >> 4, z >> 4) };
		return section && is_solid(*section, static_cast<std::uint32_t>(((y & 15) * 16 + (z & 15)) * 16 + (x & 15)));
	}
	/*
	//=====================================================================================
	*/
	bool terrain_c::is_air_at(std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		const auto* section{ get_section(x >> 4, y >> 4, z >> 4) };
		return section == nullptr || (get_state(get_block(*section, static_cast<std::uint32_t>(((y & 15) * 16 + (z & 15)) * 16 + (x & 15)))).flags & terrain_flag_air);
	}
	/*
	//=====================================================================================
	*/
	template <typename type_t>
	bool terrain_c::read_array(structures::reader_s& in, std::vector<type_t>& out, std::size_t count)
	{
		auto ok{ false };

		if (in.good() && in.remaining() >= count * sizeof(type_t))
		{
			auto start{ out.size() };
			out.resize(start + count);
			ok = functions::read_raw(in, out.data() + start, count * sizeof(type_t));
		}

		else
		{
			in.valid = false;
		}

		return ok;
	}
}

//=====================================================================================
