
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class npc_c
	{
	public:

		void load();
		void show(structures::player_s& viewer);
		void look(structures::player_s& viewer);
		void hide(structures::player_s& viewer);
		void interact(structures::player_s& player, std::int32_t entity_id);
		void click(structures::player_s& player, std::uint32_t slot);
		void choose(structures::player_s& player, std::uint32_t button);
		const std::vector<structures::npc_s>& get_npcs() const;

	private:

		void add_npc(std::string_view line);
		void add_item(std::string_view line);
		void run(structures::player_s& player, std::string_view action);
		std::FILE* create_default(const std::string& path) const;

		static std::vector<std::string_view> split(std::string_view text, char separator);
		static std::string_view trim(std::string_view text);

		std::vector<structures::npc_s> npcs;
	};

	extern npc_c npc;
}

//=====================================================================================
