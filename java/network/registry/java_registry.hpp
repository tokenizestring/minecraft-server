
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_registry_c
	{
	public:

		bool load();
		const structures::registry_s& get(structures::family_e family) const;

	private:

		bool load_family(structures::family_e family);
		void begin_config(structures::registry_s& registry, const structures::layout_s& layout);
		void flush_section(structures::registry_s& registry);
		void write_tag_line(std::string_view line);

		std::array<structures::registry_s, structures::family_count> registries;
		std::vector<std::uint8_t> packet;
		std::vector<std::uint8_t> tags;
		std::vector<std::string_view> lines;
		std::vector<std::int32_t> ids;
		std::string_view section;
		std::uint32_t tag_registries;
		std::uint32_t tag_count;
		bool section_tags;
	};

	extern java_registry_c java_registry;
}

//=====================================================================================
