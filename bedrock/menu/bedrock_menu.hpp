
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_menu_c
	{
	public:

		void build();
		void open(structures::session_s& session, std::uint32_t index);
		void handle_response(structures::session_s& session, structures::reader_s& in);
		void handle_transaction(structures::session_s& session, structures::reader_s& in);

	private:

		std::vector<std::string> forms;
		std::vector<std::uint8_t> body;
	};

	extern bedrock_menu_c bedrock_menu;
}

//=====================================================================================
