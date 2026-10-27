
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_registry_c java_registry;

	bool java_registry_c::load()
	{
		return (load_family(structures::family_26_2) && load_family(structures::family_26_3));
	}
	/*
	//=====================================================================================
	*/
	const structures::registry_s& java_registry_c::get(structures::family_e family) const
	{
		return registries[family];
	}
	/*
	//=====================================================================================
	*/
	bool java_registry_c::load_family(structures::family_e family)
	{
		auto layout{ functions::get_layout(family) };
		auto& target{ registries[family] };
		std::string text{};

		if (auto file{ std::fopen(functions::get_local_path(layout.data_path).c_str(), "rb") }; file)
		{
			std::array<char, 65536u> block{};

			for (auto count{ std::fread(block.data(), 1u, block.size(), file) }; count; count = std::fread(block.data(), 1u, block.size(), file))
			{
				text.append(block.data(), count);
			}

			std::fclose(file);
		}

		if (text.size())
		{
			begin_config(target, layout);

			std::string_view rest{ text };

			while (rest.size())
			{
				auto end{ rest.find('\n') };
				auto line{ rest.substr(0u, end) };
				rest = (end == std::string_view::npos) ? std::string_view{} : rest.substr(end + 1u);

				if (line.starts_with(data_registry_header) || line.starts_with(data_tags_header))
				{
					flush_section(target);
					section_tags = line.starts_with(data_tags_header);
					section = line.substr(line.find(' ') + 1u);
				}

				else if (line.size())
				{
					lines.push_back(line);
				}
			}

			flush_section(target);

			packet.clear();
			functions::write_varint(packet, layout.config_out_tags);
			functions::write_varint(packet, static_cast<std::int32_t>(tag_registries));
			functions::write_bytes(packet, tags.data(), tags.size());
			functions::write_packet_frame(target.config, packet);

			packet.clear();
			functions::write_varint(packet, config_out_finish);
			functions::write_packet_frame(target.config, packet);

			target.loaded = (target.plains != missing_index && target.overworld != missing_index);

			utils::log.write(color_config, "[config](load) '%s' %u tags in %u registries, plains=%u overworld=%u, %.1f kb", layout.pack_version, tag_count, tag_registries, target.plains, target.overworld, static_cast<std::double_t>(target.config.size()) / 1024.0);
		}

		if (target.loaded)
		{
			return true;
		}

		utils::log.write(color_error, "[config](fail) '%s' missing or incomplete", layout.data_path);
		return false;
	}
	/*
	//=====================================================================================
	*/
	void java_registry_c::begin_config(structures::registry_s& target, const structures::layout_s& layout)
	{
		target.config.clear();
		target.plains = missing_index;
		target.overworld = missing_index;
		target.loaded = false;

		tags.clear();
		lines.clear();
		section = {};
		section_tags = false;
		tag_registries = 0u;
		tag_count = 0u;

		packet.clear();
		functions::write_varint(packet, config_out_custom_payload);
		functions::write_string(packet, modern_brand_channel);
		functions::write_string(packet, server_brand);
		functions::write_packet_frame(target.config, packet);

		packet.clear();
		functions::write_varint(packet, layout.config_out_features);
		functions::write_varint(packet, 1);
		functions::write_string(packet, modern_feature_vanilla);
		functions::write_packet_frame(target.config, packet);

		packet.clear();
		functions::write_varint(packet, layout.config_out_known_packs);
		functions::write_varint(packet, 1);
		functions::write_string(packet, modern_pack_namespace);
		functions::write_string(packet, modern_pack_id);
		functions::write_string(packet, layout.pack_version);
		functions::write_packet_frame(target.config, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_registry_c::flush_section(structures::registry_s& target)
	{
		if (section.size() && section_tags)
		{
			functions::write_string(tags, section);
			functions::write_varint(tags, static_cast<std::int32_t>(lines.size()));

			for (auto line : lines)
			{
				write_tag_line(line);
			}

			tag_count += static_cast<std::uint32_t>(lines.size());
			++tag_registries;
		}

		else if (section.size())
		{
			packet.clear();
			functions::write_varint(packet, config_out_registry_data);
			functions::write_string(packet, section);
			functions::write_varint(packet, static_cast<std::int32_t>(lines.size()));

			for (auto index{ 0u }; index < lines.size(); ++index)
			{
				functions::write_string(packet, lines[index]);
				functions::write_bool(packet, false);

				if (section == modern_biome_registry && lines[index] == modern_plains)
				{
					target.plains = index;
				}

				else if (section == modern_dimension_registry && lines[index] == modern_overworld)
				{
					target.overworld = index;
				}
			}

			functions::write_packet_frame(target.config, packet);
		}

		lines.clear();
	}
	/*
	//=====================================================================================
	*/
	void java_registry_c::write_tag_line(std::string_view line)
	{
		auto space{ line.find(' ') };
		ids.clear();

		for (auto cursor{ space }; cursor != std::string_view::npos; cursor = line.find(' ', cursor + 1u))
		{
			std::int32_t value{ 0 };
			std::from_chars(line.data() + cursor + 1u, line.data() + line.size(), value);
			ids.push_back(value);
		}

		functions::write_string(tags, line.substr(0u, space));
		functions::write_varint(tags, static_cast<std::int32_t>(ids.size()));

		for (auto value : ids)
		{
			functions::write_varint(tags, value);
		}
	}
}

//=====================================================================================
