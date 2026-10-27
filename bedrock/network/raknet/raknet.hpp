
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class raknet_c
	{
	public:

		bool start(HANDLE port);
		void complete(structures::datagram_s& datagram, std::size_t size, bool ok);
		void flush();
		void tick(std::uint64_t now);
		void shutdown();

		void send(structures::session_s& session, const std::vector<std::uint8_t>& payload, std::uint8_t reliability);
		void close(structures::session_s& session, const char* reason);
		std::uint32_t get_online() const;

	private:

		void post_receive(structures::datagram_s& datagram);
		void handle_offline(const sockaddr_in& from, structures::reader_s& in, std::uint8_t id);
		void handle_datagram(structures::session_s& session, structures::reader_s& in);
		void handle_acks(structures::session_s& session, structures::reader_s& in, bool nack);
		void handle_frame(structures::session_s& session, structures::reader_s& in);
		void handle_ordered(structures::session_s& session, std::uint32_t order_index, const std::uint8_t* data, std::size_t size);
		void handle_message(structures::session_s& session, const std::uint8_t* data, std::size_t size);
		void write_frame(structures::session_s& session, const std::uint8_t* data, std::size_t size, std::uint8_t reliability, std::uint32_t order_index, std::uint32_t split_count, std::uint16_t split_id, std::uint32_t split_index);
		void send_datagram(structures::session_s& session);
		void send_acks(structures::session_s& session);
		void resend(structures::session_s& session, std::uint32_t sequence);
		void send_to(const sockaddr_in& address, const std::vector<std::uint8_t>& data);
		void queue(structures::session_s& session);

		SOCKET handle;
		std::uint64_t guid;
		std::array<structures::datagram_s, bedrock_receives> receives;
		std::unordered_map<std::uint64_t, std::unique_ptr<structures::session_s>> sessions;
		std::vector<structures::session_s*> dirty;
		std::vector<std::uint32_t> expired;
		std::vector<std::uint8_t> packet;
		std::vector<std::uint8_t> message;
	};

	extern raknet_c raknet;
}

//=====================================================================================
