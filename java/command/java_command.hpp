
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_command_c
	{
	public:

		void build();
		void send_tree(structures::client_s& client);

	private:

		static void write_node(std::vector<std::uint8_t>& out, std::uint8_t flags, std::uint32_t first_child, std::uint32_t child_count, std::string_view name);

		std::array<std::vector<std::uint8_t>, structures::family_count> trees;
		std::vector<std::uint8_t> packet;
	};

	extern java_command_c java_command;
}

//=====================================================================================
