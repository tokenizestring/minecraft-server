
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_login_c
	{
	public:

		void handle_request_settings(structures::session_s& session, structures::reader_s& in);
		void handle_login(structures::session_s& session, structures::reader_s& in);
		void handle_pack_response(structures::session_s& session, structures::reader_s& in);

	private:

		void send_resource_packs_info(structures::session_s& session);
		void send_resource_pack_stack(structures::session_s& session);

		static void read_identity(structures::session_s& session, std::string_view text);

		std::vector<std::uint8_t> body;
	};

	extern bedrock_login_c bedrock_login;
}

//=====================================================================================
