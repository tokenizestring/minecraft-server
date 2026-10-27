
//=====================================================================================

#pragma once

#include "../../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class blocks_c
	{
	public:

		void load();
		std::uint32_t get_java(structures::family_e family, std::uint8_t block) const;
		std::uint32_t get_state(structures::family_e family, std::uint16_t kind) const;
		std::uint8_t get_filter(std::uint8_t block) const;
		bool is_fluid(std::uint8_t block) const;
		std::uint16_t find_name(const std::string& name) const;
		const std::string& get_name(std::uint16_t kind) const;
		std::uint32_t get_kind_count() const;
		std::uint8_t get_emission(std::uint16_t kind) const;
		std::uint8_t get_opacity(std::uint16_t kind) const;
		std::uint8_t get_state_emission(std::uint32_t state) const;
		std::uint32_t get_light_state(structures::family_e family, std::uint8_t level) const;
		bool is_instant(std::uint16_t kind) const;
		std::uint16_t add_kind(const std::string& name, const std::array<std::uint32_t, structures::family_count>& states, std::uint16_t like);

	private:

		void load_placed();
		void load_light();
		void load_instant();

		static std::vector<std::pair<std::string, std::uint32_t>> read_table(const char* path, bool legacy);

		std::array<std::array<std::uint32_t, structures::block_count>, structures::family_count> ids;
		std::array<std::uint8_t, 256u> filters;
		std::array<bool, 256u> fluids;
		std::array<std::vector<std::uint32_t>, structures::family_count> placed;
		std::unordered_map<std::string, std::uint16_t> names;
		std::vector<std::string> kind_names;
		std::vector<std::uint8_t> state_emission;
		std::vector<std::uint8_t> kind_emission;
		std::vector<std::uint8_t> kind_opacity;
		std::vector<std::uint8_t> kind_instant;
		std::uint16_t light_kind;
	};

	extern blocks_c blocks;
}

//=====================================================================================
