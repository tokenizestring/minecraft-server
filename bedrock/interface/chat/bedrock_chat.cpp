
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_chat_c bedrock_chat;

	void bedrock_chat_c::handle(structures::session_s& session, structures::reader_s& in)
	{
		functions::read_u8(in);
		auto variant{ static_cast<std::uint32_t>(functions::read_varint(in)) };
		auto type{ functions::read_u8(in) };
		functions::read_string(in, max_chat_length);
		auto message{ functions::read_string(in, max_chat_length) };

		if (in.good() && session.playing && variant == bedrock_text_author_message && type == bedrock_text_chat)
		{
			chat.say(session.player, message);
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chat_c::handle_command(structures::session_s& session, structures::reader_s& in)
	{
		if (auto line{ functions::read_string(in, max_chat_length) }; in.good() && session.playing)
		{
			command.run(session.player, line);
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chat_c::notify(structures::session_s& session, std::string_view message)
	{
		body.clear();
		functions::write_bool(body, false);
		functions::write_varint(body, static_cast<std::int32_t>(bedrock_text_message_only));
		functions::write_u8(body, bedrock_text_tip);
		functions::write_string(body, message);
		functions::write_string(body, "");
		functions::write_string(body, "");
		functions::write_string(body, "");
		bedrock_packet.send(session, bedrock_text, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chat_c::prepare(const std::vector<std::string>& lines)
	{
		count = static_cast<std::uint32_t>(lines.size());
		board.clear();
		functions::write_u8(board, raknet_game_packet);
		functions::write_u8(board, bedrock_compression_none);

		body.clear();
		functions::write_varint(body, static_cast<std::int32_t>(bedrock_set_display_objective));
		functions::write_string(body, bedrock_display_sidebar);
		functions::write_string(body, chat_objective);
		functions::write_string(body, chat_title);
		functions::write_string(body, bedrock_criteria_dummy);
		functions::write_zigzag32(body, bedrock_sort_descending);
		functions::write_varint(board, static_cast<std::int32_t>(body.size()));
		functions::write_bytes(board, body.data(), body.size());

		body.clear();
		functions::write_varint(body, static_cast<std::int32_t>(bedrock_set_score));
		functions::write_varint(body, static_cast<std::int32_t>(lines.size()));

		for (auto index{ 0u }; index < lines.size(); ++index)
		{
			next_score_id += 1;
			functions::write_varint(body, static_cast<std::int32_t>(bedrock_score_fake_player));
			functions::write_string(body, bedrock_score_fake_player_name);
			functions::write_zigzag64(body, next_score_id);
			functions::write_string(body, chat_objective);
			functions::write_u32_le(body, config.chat_lines - index);
			functions::write_string(body, lines[index]);
		}

		functions::write_varint(board, static_cast<std::int32_t>(body.size()));
		functions::write_bytes(board, body.data(), body.size());
	}
	/*
	//=====================================================================================
	*/
	void bedrock_chat_c::show(structures::session_s& session, bool visible)
	{
		if (session.player.sidebar)
		{
			body.clear();
			functions::write_string(body, chat_objective);
			bedrock_packet.send(session, bedrock_remove_objective, body);
			session.player.sidebar = false;
		}

		if (visible && count)
		{
			raknet.send(session, board, raknet_reliability_reliable_ordered);
			session.player.sidebar = true;
		}
	}
}

//=====================================================================================
