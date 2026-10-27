
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_play_c
	{
	public:

		void start_game(structures::session_s& session);
		void handle_chunk_radius(structures::session_s& session, structures::reader_s& in);
		void handle_auth_input(structures::session_s& session, structures::reader_s& in);
		void join(structures::session_s& session);
		void leave(structures::session_s& session);

	private:

		void teleport(structures::session_s& session);

		static void write_start_game(std::vector<std::uint8_t>& out, const structures::session_s& session);

		std::vector<std::uint8_t> body;
	};

	extern bedrock_play_c bedrock_play;
}

//=====================================================================================
