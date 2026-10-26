
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_chat_c java_chat;

	void java_chat_c::build()
	{
		clears[structures::family_26_2] = render_modern(structures::family_26_2, {});
		clears[structures::family_26_3] = render_modern(structures::family_26_3, {});
		prepare({});
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::handle(structures::client_s& client, structures::reader_s& in)
	{
		if (auto message{ functions::read_string(in, max_chat_length) }; in.good() && message.size() && message[0] == '/')
		{
			command.run(client.player, message);
		}

		else if (in.good())
		{
			chat.say(client.player, message);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::handle_command(structures::client_s& client, structures::reader_s& in)
	{
		if (auto text{ functions::read_string(in, max_chat_length) }; in.good())
		{
			command.run(client.player, text);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::notify(structures::client_s& client, std::string_view message)
	{
		packet.clear();

		if (functions::is_modern(client.family))
		{
			functions::write_varint(packet, client.layout.play_out_system_chat);
			functions::write_nbt_text(packet, message);
			functions::write_bool(packet, true);
		}

		else
		{
			functions::write_varint(packet, v1_8_play_out_chat);
			functions::write_json_text(packet, message);
			functions::write_u8(packet, v1_8_chat_action_bar);
		}

		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::open(structures::client_s& client)
	{
		if (functions::is_modern(client.family))
		{
			packet.clear();
			functions::write_varint(packet, client.layout.play_out_set_objective);
			functions::write_string(packet, chat_objective);
			functions::write_u8(packet, objective_create);
			functions::write_nbt_text(packet, chat_title);
			functions::write_varint(packet, 0);
			functions::write_bool(packet, true);
			functions::write_varint(packet, number_format_blank);
			network.send(client, packet);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::prepare(const std::vector<std::string>& lines)
	{
		count = static_cast<std::uint32_t>(lines.size());
		boards[structures::family_26_2] = render_modern(structures::family_26_2, lines);
		boards[structures::family_26_3] = render_modern(structures::family_26_3, lines);
		boards[structures::family_1_8] = render_1_8(lines);
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::show(structures::client_s& client, bool visible)
	{
		if (functions::is_modern(client.family))
		{
			show_modern(client, visible ? count : 0u);
		}

		else
		{
			show_1_8(client, visible ? count : 0u);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::show_modern(structures::client_s& client, std::uint32_t shown)
	{
		if (shown && client.player.sidebar)
		{
			network.send_held(client, boards[client.family]);
		}

		else if (shown)
		{
			send_display(client, chat_objective);
			network.send_held(client, boards[client.family]);
			client.player.sidebar = true;
		}

		else if (client.player.sidebar)
		{
			send_display(client, "");
			network.send_held(client, clears[client.family]);
			client.player.sidebar = false;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::show_1_8(structures::client_s& client, std::uint32_t shown)
	{
		if (client.player.sidebar)
		{
			packet.clear();
			functions::write_varint(packet, v1_8_play_out_objective);
			functions::write_string(packet, chat_objective);
			functions::write_u8(packet, objective_remove);
			network.send(client, packet);
			client.player.sidebar = false;
		}

		if (shown)
		{
			network.send_held(client, boards[structures::family_1_8]);
			client.player.sidebar = true;
		}
	}
	/*
	//=====================================================================================
	*/
	void java_chat_c::send_display(structures::client_s& client, std::string_view objective)
	{
		packet.clear();
		functions::write_varint(packet, client.layout.play_out_set_display_objective);
		functions::write_varint(packet, display_slot_sidebar);
		functions::write_string(packet, objective);
		network.send(client, packet);
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<std::uint8_t>> java_chat_c::render_modern(structures::family_e family, const std::vector<std::string>& lines)
	{
		auto out{ std::make_shared<std::vector<std::uint8_t>>() };
		const auto& layout{ functions::get_layout(family) };

		for (auto index{ 0u }; index < config.chat_lines; ++index)
		{
			line.assign("line").append(std::to_string(index));
			packet.clear();

			if (index < lines.size())
			{
				functions::write_varint(packet, layout.play_out_set_score);
				functions::write_string(packet, line);
				functions::write_string(packet, chat_objective);
				functions::write_varint(packet, static_cast<std::int32_t>(config.chat_lines - index));
				functions::write_bool(packet, true);
				functions::write_nbt_text(packet, lines[index]);
				functions::write_bool(packet, false);
			}

			else
			{
				functions::write_varint(packet, layout.play_out_reset_score);
				functions::write_string(packet, line);
				functions::write_bool(packet, true);
				functions::write_string(packet, chat_objective);
			}

			functions::write_frame(*out, packet);
		}

		return out;
	}
	/*
	//=====================================================================================
	*/
	std::shared_ptr<const std::vector<std::uint8_t>> java_chat_c::render_1_8(const std::vector<std::string>& lines)
	{
		auto out{ std::make_shared<std::vector<std::uint8_t>>() };

		packet.clear();
		functions::write_varint(packet, v1_8_play_out_objective);
		functions::write_string(packet, chat_objective);
		functions::write_u8(packet, objective_create);
		functions::write_string(packet, chat_title);
		functions::write_string(packet, v1_8_objective_type);
		functions::write_frame(*out, packet);

		packet.clear();
		functions::write_varint(packet, v1_8_play_out_display_objective);
		functions::write_u8(packet, static_cast<std::uint8_t>(display_slot_sidebar));
		functions::write_string(packet, chat_objective);
		functions::write_frame(*out, packet);

		for (auto index{ 0u }; index < lines.size(); ++index)
		{
			line.assign(get_1_8_text(lines[index])).append("\xC2\xA7").push_back("0123456789abcdef"[index & 15u]);

			packet.clear();
			functions::write_varint(packet, v1_8_play_out_score);
			functions::write_string(packet, line);
			functions::write_u8(packet, 0u);
			functions::write_string(packet, chat_objective);
			functions::write_varint(packet, static_cast<std::int32_t>(config.chat_lines - index));
			functions::write_frame(*out, packet);
		}

		return out;
	}
	/*
	//=====================================================================================
	*/
	std::string_view java_chat_c::get_1_8_text(std::string_view text)
	{
		auto end{ 0u };
		auto units{ 0u };

		while (end < text.size() && units + ((static_cast<std::uint8_t>(text[end]) >> 3u) == 0x1Eu ? 2u : 1u) <= v1_8_score_limit)
		{
			units += (static_cast<std::uint8_t>(text[end]) >> 3u) == 0x1Eu ? 2u : 1u;
			end += 1u;

			while (end < text.size() && (static_cast<std::uint8_t>(text[end]) >> 6u) == 0x02u)
			{
				end += 1u;
			}
		}

		return text.substr(0u, end);
	}
}

//=====================================================================================
