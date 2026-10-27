
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_command_c java_command;

	void java_command_c::build()
	{
		for (auto family : { structures::family_26_2, structures::family_26_3 })
		{
			auto chats{ static_cast<std::uint32_t>(std::size(chat_arguments)) };
			auto modes{ static_cast<std::uint32_t>(std::size(gamemode_arguments)) };

			packet.clear();
			functions::write_varint(packet, functions::get_layout(family).play_out_commands);
			functions::write_varint(packet, static_cast<std::int32_t>(16u + chats + modes));
			write_node(packet, command_node_root, 1u, 14u, "");
			write_node(packet, command_node_literal | command_node_executable, 16u, chats, chat_command);
			write_node(packet, command_node_literal, 16u + chats, modes, gamemode_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, smp_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, spawn_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, roll_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, balance_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, balance_alias);
			write_node(packet, command_node_literal, 15u, 1u, pay_command);
			write_node(packet, command_node_literal, 15u, 1u, sell_command);
			write_node(packet, command_node_literal | command_node_executable, 15u, 1u, buy_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, games_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, friends_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, info_command);
			write_node(packet, command_node_literal | command_node_executable, 0u, 0u, rtp_command);
			write_node(packet, command_node_argument | command_node_executable, 0u, 0u, "");
			functions::write_string(packet, command_text_argument);
			functions::write_varint(packet, command_parser_string);
			functions::write_varint(packet, command_string_greedy);

			for (auto argument : chat_arguments)
			{
				write_node(packet, command_node_literal | command_node_executable, 0u, 0u, argument);
			}

			for (auto argument : gamemode_arguments)
			{
				write_node(packet, command_node_literal | command_node_executable, 0u, 0u, argument);
			}

			functions::write_varint(packet, 0);
			functions::write_packet_frame(trees[family], packet);
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
