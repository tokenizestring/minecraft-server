
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_menu_c
	{
	public:

		void build();
		void open(structures::client_s& client, std::uint32_t index);
		void handle_interact(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void click(structures::client_s& client, std::int32_t slot);
		void handle_close(structures::client_s& client);
		void handle_use_entity_1_8(structures::client_s& client, structures::reader_s& in);
		void transfer(structures::client_s& client, std::string_view host, std::uint32_t port);

	private:

		std::shared_ptr<const std::vector<std::uint8_t>> render_screen(structures::family_e family, const structures::npc_s& entry);
		std::shared_ptr<const std::vector<std::uint8_t>> render_content(structures::family_e family, const structures::npc_s& entry);
		void write_item_modern(std::vector<std::uint8_t>& out, structures::family_e family, const structures::menu_item_s& item) const;
		void write_item_1_8(std::vector<std::uint8_t>& out, const structures::menu_item_s& item) const;

		static std::unordered_map<std::string, std::int32_t> load_items(const char* path);

		std::array<std::vector<std::shared_ptr<const std::vector<std::uint8_t>>>, structures::family_count> screens;
		std::array<std::vector<std::shared_ptr<const std::vector<std::uint8_t>>>, structures::family_count> contents;
		std::array<std::unordered_map<std::string, std::int32_t>, structures::family_count> items;
		std::vector<std::uint8_t> packet;
	};

	extern java_menu_c java_menu;
}

//=====================================================================================
