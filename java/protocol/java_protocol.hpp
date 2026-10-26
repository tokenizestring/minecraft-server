
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_protocol_c
	{
	public:

		void handle(structures::client_s& client, structures::reader_s& frame);
		void handle_legacy(structures::client_s& client);
		void kick(structures::client_s& client, std::string_view reason);

	private:

		void handle_handshake(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void handle_status(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void handle_login(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void handle_configuration(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void start_login(structures::client_s& client, structures::reader_s& in);
		void send_login_success(structures::client_s& client);

		std::vector<std::uint8_t> packet;
	};

	extern java_protocol_c java_protocol;
}

//=====================================================================================
