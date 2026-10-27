
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_packet_c
	{
	public:

		void handle(structures::session_s& session, const std::uint8_t* data, std::size_t size);
		void send(structures::session_s& session, std::uint32_t id, const std::vector<std::uint8_t>& payload);
		void send_play_status(structures::session_s& session, std::int32_t status);
		void kick(structures::session_s& session, std::string_view reason);

	private:

		void handle_packet(structures::session_s& session, std::uint32_t id, structures::reader_s& in);
		void handle_violation(structures::session_s& session, structures::reader_s& in);

		std::vector<std::uint8_t> inflated;
		std::vector<std::uint8_t> batch;
		std::vector<std::uint8_t> packet;
		std::vector<std::uint8_t> body;
	};

	extern bedrock_packet_c bedrock_packet;
}

//=====================================================================================
