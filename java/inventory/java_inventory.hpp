
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_inventory_c
	{
	public:

		void send_all(structures::client_s& client);
		void send_slot(structures::client_s& client, std::uint32_t slot);
		void handle_click(structures::client_s& client, structures::reader_s& in);
		void handle_click_1_8(structures::client_s& client, structures::reader_s& in);
		void handle_creative(structures::client_s& client, structures::reader_s& in);
		void handle_creative_1_8(structures::client_s& client, structures::reader_s& in);
		void handle_held(structures::client_s& client, structures::reader_s& in);
		void swap_hands(structures::client_s& client);
		void drop(structures::client_s& client, bool all);

		static void write_item(std::vector<std::uint8_t>& out, structures::family_e family, const structures::item_stack_s& stack);

	private:

		void click_window(structures::client_s& client, std::int32_t window, std::int32_t slot, std::int32_t button, std::int32_t mode);
		static structures::item_stack_s read_hashed(structures::family_e family, structures::reader_s& in);
		static bool same(const structures::item_stack_s& a, const structures::item_stack_s& b);

		std::vector<std::uint8_t> packet;
	};

	extern java_inventory_c java_inventory;
}

//=====================================================================================
