
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class hub_c
	{
	public:

		void load();
		void open(structures::player_s& player, std::uint32_t menu);
		void use(structures::player_s& player);
		void click(structures::player_s& player, std::uint32_t slot);
		void choose(structures::player_s& player, std::uint32_t button);
		const std::array<structures::menu_s, hub_menu_count>& get_menus() const;
		const structures::menu_item_s* get_hotbar(std::uint32_t slot) const;

	private:

		void add_menu(std::string_view line);
		void add_item(std::string_view line);
		void run(structures::player_s& player, std::string_view action);
		std::FILE* create_default(const std::string& path) const;

		static std::vector<std::string_view> split(std::string_view text, char separator);
		static std::string_view trim(std::string_view text);

		std::array<structures::menu_s, hub_menu_count> menus;
		std::array<structures::menu_item_s, hotbar_size> hotbar;
		std::uint32_t current;
	};

	extern hub_c hub;
}

//=====================================================================================
