
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_market_c
	{
	public:

		void open(structures::client_s& client, std::string_view filter);
		void click(structures::client_s& client, std::int32_t slot, std::int32_t button, std::int32_t mode);

		static void write_entry(std::vector<std::uint8_t>& out, structures::family_e family, const structures::item_stack_s& stack, std::string_view name, const std::vector<std::string>& lore);

	private:

		void render(structures::client_s& client);

		std::vector<std::uint8_t> packet;
	};

	extern java_market_c java_market;
}

//=====================================================================================
