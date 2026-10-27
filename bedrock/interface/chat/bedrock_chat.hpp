
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_chat_c
	{
	public:

		void handle(structures::session_s& session, structures::reader_s& in);
		void handle_command(structures::session_s& session, structures::reader_s& in);
		void notify(structures::session_s& session, std::string_view message);
		void prepare(const std::vector<std::string>& lines);
		void show(structures::session_s& session, bool visible);

	private:

		std::vector<std::uint8_t> body;
		std::vector<std::uint8_t> board;
		std::int64_t next_score_id;
		std::uint32_t count;
	};

	extern bedrock_chat_c bedrock_chat;
}

//=====================================================================================
