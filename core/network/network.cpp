
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	network_c network;

	bool network_c::start()
	{
		WSADATA data{};

		if (WSAStartup(MAKEWORD(2u, 2u), &data) == ERROR_SUCCESS)
		{
			sockaddr_in address{};
			address.sin_family = AF_INET;
			address.sin_port = htons(config.java_port);
			inet_pton(AF_INET, server_address, &address.sin_addr);

			listener = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0u, WSA_FLAG_OVERLAPPED);
			port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0u, 1u);

			BOOL exclusive{ TRUE };
			setsockopt(listener, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));

			GUID guid = WSAID_ACCEPTEX;
			DWORD returned{ 0u };

			if (listener != INVALID_SOCKET && port && bind(listener, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != SOCKET_ERROR && listen(listener, SOMAXCONN) != SOCKET_ERROR && CreateIoCompletionPort(reinterpret_cast<HANDLE>(listener), port, 0u, 0u) && WSAIoctl(listener, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &accept_ex, sizeof(accept_ex), &returned, nullptr, nullptr) != SOCKET_ERROR)
			{
				scratch.resize(receive_size);
				buffers.reserve(max_send_buffers);
				running = true;

				auto posted{ static_cast<std::uint32_t>(std::count_if(accepts.begin(), accepts.end(), [this](auto& operation) { return post_accept(operation); })) };

				utils::log.write(color_net, "[net](listen) '%s:%u' iocp with %u accepts posted", server_address, config.java_port, posted);
				return (posted == accept_backlog && raknet.start(port));
			}

			utils::log.write(color_error, "[net](fail) could not bind '%s:%u' error=%d", server_address, config.java_port, WSAGetLastError());
		}

		return false;
	}
	/*
	//=====================================================================================
	*/
	void network_c::run()
	{
		std::array<OVERLAPPED_ENTRY, completion_batch> entries{};
		auto next_tick{ functions::get_time_ms() };

		while (running)
		{
			auto now{ functions::get_time_ms() };
			ULONG count{ 0u };

			if (GetQueuedCompletionStatusEx(port, entries.data(), static_cast<ULONG>(entries.size()), &count, next_tick > now ? static_cast<DWORD>(next_tick - now) : 0u, FALSE))
			{
				for (auto index{ 0u }; index < count; ++index)
				{
					complete(entries[index]);
				}
			}

			if (now = functions::get_time_ms(); now >= next_tick)
			{
				tick(now);
				raknet.tick(now);
				world.tick();
				java_chunk.tick();
				chat.tick(now);
				next_tick = now + tick_ms;
			}

			raknet.flush();

			for (auto client : dirty)
			{
				client->queued = false;
				post_send(*client);
			}

			dirty.clear();

			if (dead)
			{
				sweep();
			}
		}

		shutdown();
	}
	/*
	//=====================================================================================
	*/
	void network_c::stop()
	{
		running = false;
		PostQueuedCompletionStatus(port, 0u, 0u, nullptr);
	}
	/*
	//=====================================================================================
	*/
	void network_c::send(structures::client_s& client, const std::vector<std::uint8_t>& body)
	{
		if (client.active() && client.tx_bytes + body.size() < max_backlog)
		{
			auto& tail{ get_tail(client) };
			auto before{ tail.size() };

			if (client.compressed && body.size() >= java_compression_threshold)
			{
				squeezed.clear();
				deflater.compress(body.data(), body.size(), squeezed, true);
				functions::write_varint(tail, static_cast<std::int32_t>(functions::get_varint_size(static_cast<std::int32_t>(body.size())) + squeezed.size()));
				functions::write_varint(tail, static_cast<std::int32_t>(body.size()));
				functions::write_bytes(tail, squeezed.data(), squeezed.size());
			}

			else if (client.compressed)
			{
				functions::write_packet_frame(tail, body);
			}

			else
			{
				functions::write_frame(tail, body);
			}

			client.tx_bytes += tail.size() - before;
			queue(client);
		}

		else if (client.active())
		{
			close(client, "transmit backlog exceeded");
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::send_raw(structures::client_s& client, const std::uint8_t* data, std::size_t size)
	{
		if (client.active() && client.tx_bytes + size < max_backlog)
		{
			functions::write_bytes(get_tail(client), data, size);
			client.tx_bytes += size;
			queue(client);
		}

		else if (client.active())
		{
			close(client, "transmit backlog exceeded");
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::send_shared(structures::client_s& client, const std::vector<std::uint8_t>& data)
	{
		if (client.active() && client.tx_bytes + data.size() < max_backlog)
		{
			client.tx.push_back(structures::segment_s{ data.data(), data.size(), {} });
			client.tx_bytes += data.size();
			queue(client);
		}

		else if (client.active())
		{
			close(client, "transmit backlog exceeded");
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::send_held(structures::client_s& client, const std::shared_ptr<const std::vector<std::uint8_t>>& data)
	{
		if (client.active() && client.tx_bytes + data->size() < max_backlog)
		{
			client.tx.push_back(structures::segment_s{ data->data(), data->size(), {}, data });
			client.tx_bytes += data->size();
			queue(client);
		}

		else if (client.active())
		{
			close(client, "transmit backlog exceeded");
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::finish(structures::client_s& client)
	{
		if (client.active())
		{
			client.closing = true;
			client.closing_ms = functions::get_time_ms();
			queue(client);
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::close(structures::client_s& client, const char* reason)
	{
		if (client.alive)
		{
			closesocket(client.handle);
			client.alive = false;
			++dead;

			if (reason)
			{
				utils::log.write(color_warn, "[net](drop) '%s' %s", client.named() ? client.name.c_str() : client.address.c_str(), reason);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t network_c::get_online() const
	{
		return static_cast<std::uint32_t>(std::count_if(clients.begin(), clients.end(), [](const auto& client) { return (client->alive && client->playing()); }));
	}
	/*
	//=====================================================================================
	*/
	bool network_c::is_name_taken(const structures::client_s& client) const
	{
		return std::any_of(clients.begin(), clients.end(), [&client](const auto& other) { return (other.get() != &client && other->alive && other->name == client.name); });
	}
	/*
	//=====================================================================================
	*/
	bool network_c::post_accept(structures::operation_s& operation)
	{
		operation.overlapped = {};
		operation.type = structures::operation_accept;
		operation.client = nullptr;
		operation.handle = WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, nullptr, 0u, WSA_FLAG_OVERLAPPED);

		DWORD received{ 0u };

		if (operation.handle != INVALID_SOCKET && (accept_ex(listener, operation.handle, operation.addresses.data(), 0u, sizeof(sockaddr_in) + 16u, sizeof(sockaddr_in) + 16u, &received, &operation.overlapped) || WSAGetLastError() == ERROR_IO_PENDING))
		{
			return true;
		}

		utils::log.write(color_error, "[net](fail) accept post error=%d", WSAGetLastError());
		return false;
	}
	/*
	//=====================================================================================
	*/
	void network_c::post_receive(structures::client_s& client)
	{
		WSABUF empty{ 0u, nullptr };
		DWORD flags{ 0u };

		client.receive_operation.overlapped = {};

		if (WSARecv(client.handle, &empty, 1u, nullptr, &flags, &client.receive_operation.overlapped, nullptr) != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING)
		{
			++client.operations;
		}

		else
		{
			close(client, nullptr);
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::post_send(structures::client_s& client)
	{
		if (client.alive && client.pending() && !client.sending)
		{
			buffers.clear();

			for (auto index{ 0u }; index < client.tx.size() && index < max_send_buffers; ++index)
			{
				auto offset{ index ? 0u : client.tx_cursor };
				buffers.push_back(WSABUF{ static_cast<ULONG>(client.tx[index].size() - offset), reinterpret_cast<CHAR*>(const_cast<std::uint8_t*>(client.tx[index].data() + offset)) });
			}

			client.in_flight = static_cast<std::uint32_t>(buffers.size());
			client.send_operation.overlapped = {};

			if (WSASend(client.handle, buffers.data(), static_cast<DWORD>(buffers.size()), nullptr, 0u, &client.send_operation.overlapped, nullptr) != SOCKET_ERROR || WSAGetLastError() == WSA_IO_PENDING)
			{
				client.sending = true;
				++client.operations;
			}

			else
			{
				close(client, nullptr);
			}
		}

		else if (client.alive && client.closing && !client.sending)
		{
			::shutdown(client.handle, SD_SEND);
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::complete(const OVERLAPPED_ENTRY& entry)
	{
		if (auto operation{ reinterpret_cast<structures::operation_s*>(entry.lpOverlapped) }; operation)
		{
			auto ok{ entry.Internal == ERROR_SUCCESS };

			switch (operation->type)
			{
			case structures::operation_accept:
				complete_accept(*operation, ok);
				break;

			case structures::operation_receive:
				--operation->client->operations;
				complete_receive(*operation->client, ok);
				break;

			case structures::operation_send:
				--operation->client->operations;
				complete_send(*operation->client, ok, entry.dwNumberOfBytesTransferred);
				break;

			case structures::operation_datagram:
				raknet.complete(*reinterpret_cast<structures::datagram_s*>(entry.lpOverlapped), entry.dwNumberOfBytesTransferred, ok);
				break;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::complete_accept(structures::operation_s& operation, bool ok)
	{
		if (ok && running)
		{
			setsockopt(operation.handle, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char*>(&listener), sizeof(listener));

			BOOL nodelay{ TRUE };
			setsockopt(operation.handle, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>(&nodelay), sizeof(nodelay));

			sockaddr_in source{};
			std::int32_t length{ sizeof(source) };
			getpeername(operation.handle, reinterpret_cast<sockaddr*>(&source), &length);

			std::array<char, INET_ADDRSTRLEN> text{};
			inet_ntop(AF_INET, &source.sin_addr, text.data(), text.size());

			auto client{ std::make_unique<structures::client_s>() };
			client->handle = operation.handle;
			client->address = std::string{ text.data() }.append(":").append(std::to_string(ntohs(source.sin_port)));
			client->state = structures::state_handshake;
			client->connected_ms = functions::get_time_ms();
			client->alive = true;
			client->receive_operation.type = structures::operation_receive;
			client->receive_operation.client = client.get();
			client->send_operation.type = structures::operation_send;
			client->send_operation.client = client.get();

			if (CreateIoCompletionPort(reinterpret_cast<HANDLE>(operation.handle), port, 0u, 0u))
			{
				post_receive(*client);
			}

			else
			{
				close(*client, "completion port association failed");
			}

			clients.push_back(std::move(client));
		}

		else
		{
			closesocket(operation.handle);
		}

		if (running)
		{
			post_accept(operation);
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::complete_receive(structures::client_s& client, bool ok)
	{
		if (ok && client.alive)
		{
			if (auto count{ recv(client.handle, reinterpret_cast<char*>(scratch.data()), static_cast<std::int32_t>(scratch.size()), 0) }; count > 0 && client.closing)
			{
				post_receive(client);
			}

			else if (count > 0)
			{
				functions::write_bytes(client.rx, scratch.data(), static_cast<std::size_t>(count));
				process(client);

				if (client.alive && client.rx.size() > max_frame_size * 2u)
				{
					close(client, "receive backlog exceeded");
				}

				else if (client.alive)
				{
					post_receive(client);
				}
			}

			else
			{
				close(client, nullptr);
			}
		}

		else
		{
			close(client, nullptr);
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::complete_send(structures::client_s& client, bool ok, std::size_t bytes)
	{
		client.sending = false;
		client.in_flight = 0u;

		if (ok && client.alive)
		{
			client.tx_bytes -= bytes;

			while (bytes && client.tx.size())
			{
				if (auto available{ client.tx.front().size() - client.tx_cursor }; bytes >= available)
				{
					bytes -= available;
					client.tx_cursor = 0u;
					client.tx.pop_front();
				}

				else
				{
					client.tx_cursor += bytes;
					bytes = 0u;
				}
			}

			post_send(client);
		}

		else
		{
			close(client, nullptr);
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::process(structures::client_s& client)
	{
		auto parsing{ true };

		while (parsing && client.active() && client.rx_cursor < client.rx.size())
		{
			auto available{ client.rx.size() - client.rx_cursor };

			if (client.state == structures::state_handshake && client.rx[client.rx_cursor] == legacy_ping_byte)
			{
				java_protocol.handle_legacy(client);
				client.rx_cursor = client.rx.size();
			}

			else
			{
				structures::reader_s header{ client.rx.data() + client.rx_cursor, std::min<std::size_t>(available, max_frame_header_size), 0u, true };
				auto length{ functions::read_varint(header) };

				if (header.good() && length > 0 && static_cast<std::size_t>(length) <= max_frame_size && available - header.cursor >= static_cast<std::size_t>(length))
				{
					structures::reader_s frame{ client.rx.data() + client.rx_cursor + header.cursor, static_cast<std::size_t>(length), 0u, true };
					client.rx_cursor += header.cursor + static_cast<std::size_t>(length);
					auto unpacked{ client.compressed ? functions::read_varint(frame) : 0 };

					if (unpacked > 0 && static_cast<std::size_t>(unpacked) <= max_frame_size && frame.remaining() > 2u && zlib.inflate_raw(frame.data + frame.cursor + 2u, frame.remaining() - 2u, inflated, static_cast<std::size_t>(unpacked)) && inflated.size() == static_cast<std::size_t>(unpacked))
					{
						structures::reader_s expanded{ inflated.data(), inflated.size(), 0u, true };
						java_protocol.handle(client, expanded);
					}

					else if (unpacked == 0 && frame.good())
					{
						structures::reader_s body{ frame.data + frame.cursor, frame.remaining(), 0u, true };
						java_protocol.handle(client, body);
					}

					else
					{
						close(client, "bad compressed frame");
					}
				}

				else if (header.good() && length > 0 && static_cast<std::size_t>(length) <= max_frame_size)
				{
					parsing = false;
				}

				else if (header.good() || available >= max_frame_header_size)
				{
					close(client, "malformed frame");
				}

				else
				{
					parsing = false;
				}
			}
		}

		if (client.rx_cursor < client.rx.size())
		{
			client.rx.erase(client.rx.begin(), client.rx.begin() + static_cast<std::ptrdiff_t>(client.rx_cursor));
		}

		else if (client.rx.capacity() > rx_shrink_size)
		{
			std::vector<std::uint8_t>{}.swap(client.rx);
		}

		else
		{
			client.rx.clear();
		}

		client.rx_cursor = 0u;
	}
	/*
	//=====================================================================================
	*/
	void network_c::queue(structures::client_s& client)
	{
		if (client.alive && !client.queued)
		{
			client.queued = true;
			dirty.push_back(&client);
		}
	}
	/*
	//=====================================================================================
	*/
	std::vector<std::uint8_t>& network_c::get_tail(structures::client_s& client)
	{
		if (client.tx.size() <= client.in_flight || client.tx.back().shared || client.tx.back().owned.size() >= max_owned_segment)
		{
			client.tx.emplace_back();
		}

		return client.tx.back().owned;
	}
	/*
	//=====================================================================================
	*/
	void network_c::tick(std::uint64_t now)
	{
		for (auto& client : clients)
		{
			if (client->active() && client->playing())
			{
				java_play.tick(*client, now);
			}

			else if (client->active() && now - client->connected_ms > login_timeout_ms)
			{
				close(*client, "timed out before joining");
			}

			else if (client->alive && client->closing && now - client->closing_ms > linger_timeout_ms)
			{
				close(*client, nullptr);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void network_c::sweep()
	{
		for (auto& client : clients)
		{
			if (client->playing() && !client->alive)
			{
				java_play.leave(*client);
				client->state = structures::state_handshake;
			}
		}

		std::erase_if(clients, [](const auto& client) { return client->finished(); });
		dead = static_cast<std::uint32_t>(std::count_if(clients.begin(), clients.end(), [](const auto& client) { return !client->alive; }));
	}
	/*
	//=====================================================================================
	*/
	void network_c::shutdown()
	{
		for (auto& client : clients)
		{
			if (client->active())
			{
				java_protocol.kick(*client, "server closed");
				post_send(*client);
			}
		}

		std::array<OVERLAPPED_ENTRY, completion_batch> entries{};
		auto deadline{ functions::get_time_ms() + 500ui64 };

		while (functions::get_time_ms() < deadline && std::any_of(clients.begin(), clients.end(), [](const auto& client) { return client->sending; }))
		{
			ULONG count{ 0u };

			if (GetQueuedCompletionStatusEx(port, entries.data(), static_cast<ULONG>(entries.size()), &count, 50u, FALSE))
			{
				for (auto index{ 0u }; index < count; ++index)
				{
					complete(entries[index]);
				}
			}
		}

		for (auto& client : clients)
		{
			close(*client, nullptr);
		}

		raknet.shutdown();
		raknet.flush();
		closesocket(listener);
		CloseHandle(port);
		WSACleanup();
	}
}

//=====================================================================================
