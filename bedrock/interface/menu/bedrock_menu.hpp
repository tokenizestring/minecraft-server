
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_menu_c
	{
	public:

		void build();
		void open(structures::session_s& session, std::uint32_t index);
		void open_social(structures::session_s& session);
		void handle_response(structures::session_s& session, structures::reader_s& in);

	private:

		void send(structures::session_s& session, std::uint32_t id, std::string_view form);

		std::vector<std::string> forms;
		std::vector<std::uint8_t> body;
	};

	extern bedrock_menu_c bedrock_menu;
}

//=====================================================================================
