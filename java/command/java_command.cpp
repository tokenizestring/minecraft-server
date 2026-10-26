
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_command_c java_command;

	void java_command_c::build()
	{
		for (auto family : { structures::family_26_2, structures::family_26_3 })
		{
			auto arguments{ static_cast<std::uint32_t>(std::size(chat_arguments)) };

			packet.clear();
			functions::write_varint(packet, functions::get_layout(family).play_out_commands);
			functions::write_varint(packet, static_cast<std::int32_t>(2u + arguments));
			write_node(packet, command_node_root, 1u, 1u, "");
			write_node(packet, command_node_literal | command_node_executable, 2u, arguments, chat_command);

			for (auto argument : chat_arguments)
			{
				write_node(packet, command_node_literal | command_node_executable, 0u, 0u, argument);
			}

			functions::write_varint(packet, 0);
			functions::write_frame(trees[family], packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_command_c::send_tree(structures::client_s& client)
	{
		network.send_shared(client, trees[client.family]);
	}
	/*
	//=====================================================================================
	*/
	void java_command_c::write_node(std::vector<std::uint8_t>& out, std::uint8_t flags, std::uint32_t first_child, std::uint32_t child_count, std::string_view name)
	{
		functions::write_u8(out, flags);
		functions::write_varint(out, static_cast<std::int32_t>(child_count));

		for (auto child{ 0u }; child < child_count; ++child)
		{
			functions::write_varint(out, static_cast<std::int32_t>(first_child + child));
		}

		if (flags & command_node_literal)
		{
			functions::write_string(out, name);
		}
	}
}

//=====================================================================================
