
//=====================================================================================

#pragma once

#include "../engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class network_c
	{
	public:

		bool start();
		void run();
		void stop();

		void send(structures::client_s& client, const std::vector<std::uint8_t>& body);
		void send_raw(structures::client_s& client, const std::uint8_t* data, std::size_t size);
		void send_shared(structures::client_s& client, const std::vector<std::uint8_t>& data);
		void send_held(structures::client_s& client, const std::shared_ptr<const std::vector<std::uint8_t>>& data);
		void finish(structures::client_s& client);
		void close(structures::client_s& client, const char* reason);

		std::uint32_t get_online() const;
		bool is_name_taken(const structures::client_s& client) const;

	private:

		bool post_accept(structures::operation_s& operation);
		void post_receive(structures::client_s& client);
		void post_send(structures::client_s& client);
		void complete(const OVERLAPPED_ENTRY& entry);
		void complete_accept(structures::operation_s& operation, bool ok);
		void complete_receive(structures::client_s& client, bool ok);
		void complete_send(structures::client_s& client, bool ok, std::size_t bytes);
		void process(structures::client_s& client);
		void queue(structures::client_s& client);
		void tick(std::uint64_t now);
		void sweep();
		void shutdown();

		static std::vector<std::uint8_t>& get_tail(structures::client_s& client);

		SOCKET listener;
		HANDLE port;
		LPFN_ACCEPTEX accept_ex;
		std::atomic<bool> running;
		std::uint32_t dead;
		std::array<structures::operation_s, accept_backlog> accepts;
		std::vector<std::unique_ptr<structures::client_s>> clients;
		std::vector<structures::client_s*> dirty;
		std::vector<WSABUF> buffers;
		std::vector<std::uint8_t> scratch;
		std::vector<std::uint8_t> squeezed;
		std::vector<std::uint8_t> inflated;
		deflate_c deflater;
	};

	extern network_c network;
}

//=====================================================================================
