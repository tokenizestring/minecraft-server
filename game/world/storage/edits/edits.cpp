
//=====================================================================================

#include "../../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	edits_c edits;

	void edits_c::load()
	{
		std::vector<std::uint8_t> file{};
		std::uint64_t seed{};
		auto total{ 0u };
		last_save = functions::get_time_ms();

		if (auto handle{ std::fopen(functions::get_local_path(edits_path).c_str(), "rb") }; handle)
		{
			std::fseek(handle, 0, SEEK_END);
			file.resize(static_cast<std::size_t>(std::ftell(handle)));
			std::fseek(handle, 0, SEEK_SET);
			file.resize(std::fread(file.data(), 1u, file.size(), handle));
			std::fclose(handle);
		}

		structures::reader_s in{ file.data(), file.size(), 0u, true };
		auto magic{ functions::read_u32_le(in) };
		auto version{ functions::read_u32_le(in) };
		functions::read_raw(in, &seed, sizeof(seed));
		auto table_size{ functions::read_u32_le(in) };
		auto usable{ in.good() && magic == edits_magic && version == edits_version && seed == config.smp_seed && table_size <= in.remaining() / 3u };
		std::vector<std::uint16_t> table(usable ? table_size : 0u, kind_none);

		for (auto& entry : table)
		{
			std::uint16_t kind{};
			functions::read_raw(in, &kind, sizeof(kind));
			std::string name{ functions::read_string(in, max_kind_name) };
			entry = kind < placed_kind_base && kind < blocks.get_kind_count() && blocks.get_name(kind) == name ? kind : blocks.find_name(name);
		}

		for (auto target{ 0u }; usable && target < structures::world_count && in.good(); ++target)
		{
			auto count{ functions::read_u32_le(in) };

			for (auto chunk{ 0u }; chunk < count && in.good(); ++chunk)
			{
				std::int32_t chunk_x{};
				std::int32_t chunk_z{};
				functions::read_raw(in, &chunk_x, sizeof(chunk_x));
				functions::read_raw(in, &chunk_z, sizeof(chunk_z));
				auto size{ functions::read_u32_le(in) };
				auto list{ std::make_shared<std::vector<structures::edit_s>>() };
				list->reserve(in.good() && size <= in.remaining() / 6u ? size : 0u);

				for (auto index{ 0u }; index < size && in.good(); ++index)
				{
					std::uint16_t slot{};
					auto position{ functions::read_u32_le(in) };
					functions::read_raw(in, &slot, sizeof(slot));

					if (in.good() && slot < table.size() && table[slot] != kind_none)
					{
						list->push_back(structures::edit_s{ position, table[slot] });
					}
				}

				if (in.good() && list->size())
				{
					total += static_cast<std::uint32_t>(list->size());
					chunks[target][get_key(chunk_x, chunk_z)] = std::move(list);
				}
			}
		}

		if (file.size() && usable == false)
		{
			utils::log.write(color_server, "[edits](load) '%s' is from another seed or broken, starting clean", edits_path);
		}

		utils::log.write(color_server, "[edits](load) %u block edits", total);
	}
	/*
	//=====================================================================================
	*/
	bool edits_c::set(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind)
	{
		auto inside{ y >= world_min_y && y < world_min_y + world_height };

		if (inside)
		{
			auto& slot{ chunks[target][get_key(x >> 4, z >> 4)] };
			auto list{ slot ? std::make_shared<std::vector<structures::edit_s>>(*slot) : std::make_shared<std::vector<structures::edit_s>>() };
			auto index{ static_cast<std::uint32_t>(((y - world_min_y) * 16 + (z & 15)) * 16 + (x & 15)) };
			auto found{ std::find_if(list->begin(), list->end(), [index](const auto& edit) { return edit.index == index; }) };

			if (found != list->end())
			{
				found->kind = kind;
			}

			else
			{
				list->push_back(structures::edit_s{ index, kind });
			}

			slot = std::move(list);
			dirty = true;
		}

		return inside;
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<structures::edit_s>> edits_c::get(structures::world_e target, std::int32_t chunk_x, std::int32_t chunk_z) const
	{
		auto found{ chunks[target].find(get_key(chunk_x, chunk_z)) };
		return found != chunks[target].end() ? found->second : nullptr;
	}
	/*
	//=====================================================================================
	*/
	std::uint16_t edits_c::find(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z) const
	{
		auto kind{ kind_none };
		auto list{ get(target, x >> 4, z >> 4) };
		auto index{ static_cast<std::uint32_t>(((y - world_min_y) * 16 + (z & 15)) * 16 + (x & 15)) };

		if (list)
		{
			auto found{ std::find_if(list->begin(), list->end(), [index](const auto& edit) { return edit.index == index; }) };
			kind = found != list->end() ? found->kind : kind_none;
		}

		return kind;
	}
	/*
	//=====================================================================================
	*/
	void edits_c::clear(structures::world_e target)
	{
		chunks[target].clear();
		dirty = true;
	}
	/*
	//=====================================================================================
	*/
	void edits_c::tick(std::uint64_t now)
	{
		if (dirty && now - last_save >= autosave_ms && writing == false)
		{
			if (writer.joinable())
			{
				writer.join();
			}

			dirty = false;
			last_save = now;
			writing = true;
			writer = std::thread{ [this, snapshot{ chunks }, seed{ config.smp_seed }]() { write(snapshot, seed); writing = false; } };
		}
	}
	/*
	//=====================================================================================
	*/
	void edits_c::shutdown()
	{
		if (writer.joinable())
		{
			writer.join();
		}

		if (dirty)
		{
			write(chunks, config.smp_seed);
			dirty = false;
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t edits_c::get_key(std::int32_t chunk_x, std::int32_t chunk_z)
	{
		return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunk_x)) << 32u) | static_cast<std::uint32_t>(chunk_z);
	}
	/*
	//=====================================================================================
	*/
	void edits_c::write(const snapshot_t& snapshot, std::uint64_t seed)
	{
		std::vector<std::uint8_t> body{};
		std::vector<std::uint8_t> out{};
		std::vector<std::uint16_t> table{};
		std::unordered_map<std::uint16_t, std::uint16_t> slots{};
		auto start{ functions::get_time_ms() };
		auto total{ 0u };

		for (const auto& world_chunks : snapshot)
		{
			functions::write_u32_le(body, static_cast<std::uint32_t>(world_chunks.size()));

			for (const auto& [key, list] : world_chunks)
			{
				auto chunk_x{ static_cast<std::int32_t>(key >> 32u) };
				auto chunk_z{ static_cast<std::int32_t>(key & 0xFFFFFFFFu) };
				functions::write_bytes(body, &chunk_x, sizeof(chunk_x));
				functions::write_bytes(body, &chunk_z, sizeof(chunk_z));
				functions::write_u32_le(body, static_cast<std::uint32_t>(list->size()));
				total += static_cast<std::uint32_t>(list->size());

				for (const auto& edit : *list)
				{
					auto [found, added]{ slots.try_emplace(edit.kind, static_cast<std::uint16_t>(table.size())) };

					if (added)
					{
						table.push_back(edit.kind);
					}

					functions::write_u32_le(body, edit.index);
					functions::write_u16_le(body, found->second);
				}
			}
		}

		functions::write_u32_le(out, edits_magic);
		functions::write_u32_le(out, edits_version);
		functions::write_u64_le(out, seed);
		functions::write_u32_le(out, static_cast<std::uint32_t>(table.size()));

		for (auto kind : table)
		{
			functions::write_u16_le(out, kind);
			functions::write_string(out, blocks.get_name(kind));
		}

		functions::write_bytes(out, body.data(), body.size());

		if (auto file{ std::fopen(functions::get_local_path(edits_temp_path).c_str(), "wb") }; file)
		{
			auto written{ std::fwrite(out.data(), 1u, out.size(), file) == out.size() };
			std::fclose(file);

			if (written && MoveFileExA(functions::get_local_path(edits_temp_path).c_str(), functions::get_local_path(edits_path).c_str(), MOVEFILE_REPLACE_EXISTING))
			{
				utils::log.write(color_server, "[edits](save) %u block edits, %.1f kb in %llu ms", total, static_cast<std::double_t>(out.size()) / 1024.0, functions::get_time_ms() - start);
			}
		}
	}
}

//=====================================================================================
