
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	raknet_c raknet;

	bool raknet_c::start(HANDLE port)
	{
		sockaddr_in address{};
		address.sin_family = AF_INET;
		address.sin_port = htons(config.bedrock_port);
		inet_pton(AF_INET, server_address, &address.sin_addr);

		handle = WSASocketW(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0u, WSA_FLAG_OVERLAPPED);
		guid = functions::mix_bits(functions::get_time_ms() ^ reinterpret_cast<std::uintptr_t>(&raknet));

		BOOL report{ FALSE };
		DWORD returned{ 0u };
		WSAIoctl(handle, SIO_UDP_CONNRESET, &report, sizeof(report), nullptr, 0u, &returned, nullptr, nullptr);

		std::int32_t buffer_size{ 4194304 };
		setsockopt(handle, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char*>(&buffer_size), sizeof(buffer_size));
		setsockopt(handle, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char*>(&buffer_size), sizeof(buffer_size));

		if (handle != INVALID_SOCKET && bind(handle, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != SOCKET_ERROR && CreateIoCompletionPort(reinterpret_cast<HANDLE>(handle), port, 0u, 0u))
		{
			for (auto& datagram : receives)
			{
				datagram.type = structures::operation_datagram;
				post_receive(datagram);
			}

			utils::log.write(color_net, "[bedrock](listen) '%s:%u' udp protocol %d '%s'", server_address, config.bedrock_port, bedrock_protocol, bedrock_version);
			return true;
		}

		utils::log.write(color_error, "[bedrock](fail) could not bind udp '%s:%u' error=%d", server_address, config.bedrock_port, WSAGetLastError());
		return false;
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::post_receive(structures::datagram_s& datagram)
	{
		WSABUF buffer{ static_cast<ULONG>(datagram.data.size()), reinterpret_cast<CHAR*>(datagram.data.data()) };

		datagram.overlapped = {};
		datagram.flags = 0u;
		datagram.from_length = sizeof(datagram.from);

		if (WSARecvFrom(handle, &buffer, 1u, nullptr, &datagram.flags, reinterpret_cast<sockaddr*>(&datagram.from), &datagram.from_length, &datagram.overlapped, nullptr) == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			utils::log.write(color_error, "[bedrock](fail) receive post error=%d", WSAGetLastError());
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::complete(structures::datagram_s& datagram, std::size_t size, bool ok)
	{
		if (ok && size)
		{
			structures::reader_s in{ datagram.data.data(), size, 0u, true };
			auto id{ functions::read_u8(in) };

			if (auto found{ sessions.find(functions::get_address_key(datagram.from)) }; found != sessions.end() && found->second->alive && (id & raknet_flag_valid))
			{
				found->second->seen_ms = functions::get_time_ms();

				if (id & raknet_flag_ack)
				{
					handle_acks(*found->second, in, false);
				}

				else if (id & raknet_flag_nack)
				{
					handle_acks(*found->second, in, true);
				}

				else
				{
					handle_datagram(*found->second, in);
				}
			}

			else
			{
				handle_offline(datagram.from, in, id);
			}
		}

		post_receive(datagram);
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::handle_offline(const sockaddr_in& from, structures::reader_s& in, std::uint8_t id)
	{
		packet.clear();

		if (id == raknet_unconnected_ping || id == raknet_unconnected_ping_open)
		{
			auto time{ functions::read_u64(in) };

			if (functions::has_magic(in))
			{
				std::array<char, 512u> motd{};
				auto length{ std::snprintf(motd.data(), motd.size(), "MCPE;%s;%d;%s;%u;%u;%llu;%s;Survival;1;%u;%u;0;", config.motd.c_str(), bedrock_protocol, bedrock_version, network.get_online() + get_online(), config.max_players, guid, server_brand, config.bedrock_port, config.bedrock_port + 1u) };

				functions::write_u8(packet, raknet_unconnected_pong);
				functions::write_u64(packet, time);
				functions::write_u64(packet, guid);
				functions::write_bytes(packet, raknet_magic, sizeof(raknet_magic));
				functions::write_u16(packet, static_cast<std::uint16_t>(length));
				functions::write_bytes(packet, motd.data(), static_cast<std::size_t>(length));
				send_to(from, packet);
			}
		}

		else if (id == raknet_open_request_1 && functions::has_magic(in))
		{
			if (auto version{ functions::read_u8(in) }; version == raknet_protocol)
			{
				functions::write_u8(packet, raknet_open_reply_1);
				functions::write_bytes(packet, raknet_magic, sizeof(raknet_magic));
				functions::write_u64(packet, guid);
				functions::write_bool(packet, false);
				functions::write_u16(packet, static_cast<std::uint16_t>(std::clamp<std::size_t>(in.size + raknet_udp_overhead, raknet_min_mtu, raknet_max_mtu)));
			}

			else
			{
				functions::write_u8(packet, raknet_incompatible);
				functions::write_u8(packet, raknet_protocol);
				functions::write_bytes(packet, raknet_magic, sizeof(raknet_magic));
				functions::write_u64(packet, guid);
				utils::log.write(color_warn, "[bedrock](raknet) incompatible raknet version %u", version);
			}

			send_to(from, packet);
		}

		else if (id == raknet_open_request_2 && functions::has_magic(in))
		{
			if (auto family{ functions::read_u8(in) }; family == 4u)
			{
				in.cursor += 6u;
			}

			else
			{
				in.cursor += 28u;
			}

			auto mtu{ static_cast<std::uint16_t>(std::clamp<std::uint32_t>(functions::read_u16(in), raknet_min_mtu, raknet_max_mtu)) };
			auto client_guid{ functions::read_u64(in) };

			auto known{ sessions.contains(functions::get_address_key(from)) };

			if (in.good() && (known || sessions.size() < max_sessions))
			{
				functions::write_u8(packet, raknet_open_reply_2);
				functions::write_bytes(packet, raknet_magic, sizeof(raknet_magic));
				functions::write_u64(packet, guid);
				functions::write_address(packet, from);
				functions::write_u16(packet, mtu);
				functions::write_bool(packet, false);
				send_to(from, packet);
			}

			if (in.good() && sessions.size() < max_sessions && !known)
			{
				auto session{ std::make_unique<structures::session_s>() };
				session->address = from;
				session->guid = client_guid;
				session->mtu = mtu;
				session->alive = true;
				session->seen_ms = functions::get_time_ms();
				sessions[functions::get_address_key(from)] = std::move(session);

				utils::log.write(color_net, "[bedrock](open) '%s' mtu %u", functions::format_address(from).c_str(), mtu);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::handle_datagram(structures::session_s& session, structures::reader_s& in)
	{
		auto sequence{ functions::read_u24_le(in) };

		if (in.good())
		{
			session.acks.push_back(sequence);
			queue(session);

			while (in.good() && in.remaining() && session.alive)
			{
				handle_frame(session, in);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::handle_acks(structures::session_s& session, structures::reader_s& in, bool nack)
	{
		auto count{ functions::read_u16(in) };

		for (auto record{ 0u }; record < count && in.good(); ++record)
		{
			auto single{ functions::read_u8(in) == raknet_ack_single };
			auto first{ functions::read_u24_le(in) };
			auto last{ single ? first : functions::read_u24_le(in) };

			for (auto sequence{ first }; sequence <= last && sequence - first < raknet_max_ack_range && in.good(); ++sequence)
			{
				if (nack)
				{
					resend(session, sequence);
				}

				else
				{
					session.unacked.erase(sequence);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::handle_frame(structures::session_s& session, structures::reader_s& in)
	{
		auto flags{ functions::read_u8(in) };
		auto length{ (static_cast<std::size_t>(functions::read_u16(in)) + 7u) / 8u };
		auto reliability{ static_cast<std::uint8_t>(flags >> 5u) };
		auto reliable_index{ is_reliable(reliability) ? functions::read_u24_le(in) : 0u };
		auto sequence_index{ is_sequenced(reliability) ? functions::read_u24_le(in) : 0u };
		auto order_index{ is_ordered(reliability) ? functions::read_u24_le(in) : 0u };
		auto channel{ is_ordered(reliability) ? functions::read_u8(in) : 0u };
		auto split_count{ (flags & raknet_flag_split) ? functions::read_u32(in) : 0u };
		auto split_id{ (flags & raknet_flag_split) ? functions::read_u16(in) : static_cast<std::uint16_t>(0u) };
		auto split_index{ (flags & raknet_flag_split) ? functions::read_u32(in) : 0u };

		if (in.good() && in.remaining() >= length && channel == 0u && sequence_index < 0x1000000u)
		{
			auto data{ in.data + in.cursor };
			in.cursor += length;

			auto fresh{ true };

			if (is_reliable(reliability) && (reliable_index < session.reliable_floor || session.reliable_seen.contains(reliable_index)))
			{
				fresh = false;
			}

			else if (is_reliable(reliability) && reliable_index == session.reliable_floor)
			{
				++session.reliable_floor;

				while (session.reliable_seen.size() && session.reliable_seen.erase(session.reliable_floor))
				{
					++session.reliable_floor;
				}
			}

			else if (is_reliable(reliability))
			{
				session.reliable_seen.insert(reliable_index);
			}

			if (fresh && split_count && split_count <= max_split_count && split_index < split_count)
			{
				auto& split{ session.splits[split_id] };

				if (split.parts.size() != split_count)
				{
					split = structures::split_s{ split_count, 0u, reliability, order_index, std::vector<std::vector<std::uint8_t>>(split_count) };
				}

				split.received += split.parts[split_index].size() ? 0u : 1u;
				split.parts[split_index].assign(data, data + length);

				if (split.received == split.count)
				{
					std::vector<std::uint8_t> joined{};

					for (auto& part : split.parts)
					{
						joined.insert(joined.end(), part.begin(), part.end());
					}

					auto joined_reliability{ split.reliability };
					auto joined_order{ split.order_index };
					session.splits.erase(split_id);

					if (is_ordered(joined_reliability))
					{
						handle_ordered(session, joined_order, joined.data(), joined.size());
					}

					else
					{
						handle_message(session, joined.data(), joined.size());
					}
				}
			}

			else if (fresh && split_count)
			{
				close(session, "invalid split");
			}

			else if (fresh && is_ordered(reliability))
			{
				handle_ordered(session, order_index, data, length);
			}

			else if (fresh)
			{
				handle_message(session, data, length);
			}
		}

		else
		{
			in.valid = false;
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::handle_ordered(structures::session_s& session, std::uint32_t order_index, const std::uint8_t* data, std::size_t size)
	{
		if (order_index == session.expected_order)
		{
			++session.expected_order;
			handle_message(session, data, size);

			while (session.alive && session.order_queue.size() && session.order_queue.begin()->first == session.expected_order)
			{
				auto next{ std::move(session.order_queue.begin()->second) };
				session.order_queue.erase(session.order_queue.begin());
				++session.expected_order;
				handle_message(session, next.data(), next.size());
			}
		}

		else if (order_index > session.expected_order && session.order_queue.size() < raknet_max_order_queue)
		{
			session.order_queue[order_index].assign(data, data + size);
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::handle_message(structures::session_s& session, const std::uint8_t* data, std::size_t size)
	{
		structures::reader_s in{ data, size, 0u, true };
		auto id{ functions::read_u8(in) };

		message.clear();

		if (id == raknet_game_packet && session.connected)
		{
			bedrock_packet.handle(session, data + 1u, size - 1u);
		}

		else if (id == raknet_connection_request)
		{
			functions::read_u64(in);
			auto time{ functions::read_u64(in) };

			functions::write_u8(message, raknet_connection_accepted);
			functions::write_address(message, session.address);
			functions::write_u16(message, 0u);

			for (auto index{ 0u }; index < raknet_system_addresses; ++index)
			{
				functions::write_address(message, sockaddr_in{ AF_INET, 0u, {} });
			}

			functions::write_u64(message, time);
			functions::write_u64(message, functions::get_time_ms());
			send(session, message, raknet_reliability_reliable_ordered);
		}

		else if (id == raknet_new_incoming)
		{
			session.connected = true;
			utils::log.write(color_net, "[bedrock](connect) '%s'", functions::format_address(session.address).c_str());
		}

		else if (id == raknet_connected_ping)
		{
			auto time{ functions::read_u64(in) };

			functions::write_u8(message, raknet_connected_pong);
			functions::write_u64(message, time);
			functions::write_u64(message, functions::get_time_ms());
			send(session, message, raknet_reliability_unreliable);
		}

		else if (id == raknet_disconnect)
		{
			close(session, nullptr);
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::send(structures::session_s& session, const std::vector<std::uint8_t>& payload, std::uint8_t reliability)
	{
		if (session.alive)
		{
			auto order{ is_ordered(reliability) ? session.order_index++ : 0u };
			auto limit{ static_cast<std::size_t>(session.mtu) - raknet_udp_overhead - raknet_datagram_header - raknet_frame_overhead };

			if (payload.size() <= limit)
			{
				write_frame(session, payload.data(), payload.size(), reliability, order, 0u, 0u, 0u);
			}

			else
			{
				auto count{ static_cast<std::uint32_t>((payload.size() + limit - 1u) / limit) };
				auto id{ session.split_id++ };

				for (auto index{ 0u }; index < count; ++index)
				{
					write_frame(session, payload.data() + index * limit, std::min(limit, payload.size() - index * limit), raknet_reliability_reliable_ordered, order, count, id, index);
				}
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::write_frame(structures::session_s& session, const std::uint8_t* data, std::size_t size, std::uint8_t reliability, std::uint32_t order_index, std::uint32_t split_count, std::uint16_t split_id, std::uint32_t split_index)
	{
		if (session.outgoing.size() + size + raknet_frame_overhead > static_cast<std::size_t>(session.mtu) - raknet_udp_overhead)
		{
			send_datagram(session);
		}

		if (session.outgoing.size() < raknet_datagram_header)
		{
			functions::write_u8(session.outgoing, raknet_datagram_flags);
			functions::write_u24_le(session.outgoing, 0u);
		}

		functions::write_u8(session.outgoing, static_cast<std::uint8_t>((reliability << 5u) | (split_count ? raknet_flag_split : 0u)));
		functions::write_u16(session.outgoing, static_cast<std::uint16_t>(size * 8u));

		if (is_reliable(reliability))
		{
			functions::write_u24_le(session.outgoing, session.reliable_index++);
		}

		if (is_ordered(reliability))
		{
			functions::write_u24_le(session.outgoing, order_index);
			functions::write_u8(session.outgoing, 0u);
		}

		if (split_count)
		{
			functions::write_u32(session.outgoing, split_count);
			functions::write_u16(session.outgoing, split_id);
			functions::write_u32(session.outgoing, split_index);
		}

		functions::write_bytes(session.outgoing, data, size);
		queue(session);
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::send_datagram(structures::session_s& session)
	{
		if (session.outgoing.size() > raknet_datagram_header)
		{
			auto sequence{ session.send_sequence++ & 0xFFFFFFu };
			std::memcpy(session.outgoing.data() + 1u, &sequence, 3u);

			send_to(session.address, session.outgoing);
			session.unacked[sequence] = structures::resend_s{ std::move(session.outgoing), functions::get_time_ms() };
		}

		session.outgoing.clear();
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::send_acks(structures::session_s& session)
	{
		if (session.acks.size())
		{
			std::sort(session.acks.begin(), session.acks.end());
			session.acks.erase(std::unique(session.acks.begin(), session.acks.end()), session.acks.end());

			packet.clear();
			functions::write_u8(packet, static_cast<std::uint8_t>(raknet_flag_valid | raknet_flag_ack));
			functions::write_u16(packet, 0u);

			auto records{ 0u };

			for (auto index{ 0u }; index < session.acks.size(); ++records)
			{
				auto first{ session.acks[index] };
				auto last{ first };

				while (index + 1u < session.acks.size() && session.acks[index + 1u] == last + 1u)
				{
					last = session.acks[++index];
				}

				++index;

				if (first == last)
				{
					functions::write_u8(packet, raknet_ack_single);
					functions::write_u24_le(packet, first);
				}

				else
				{
					functions::write_u8(packet, raknet_ack_range);
					functions::write_u24_le(packet, first);
					functions::write_u24_le(packet, last);
				}
			}

			packet[1] = static_cast<std::uint8_t>(records >> 8u);
			packet[2] = static_cast<std::uint8_t>(records);
			send_to(session.address, packet);
			session.acks.clear();
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::resend(structures::session_s& session, std::uint32_t sequence)
	{
		if (auto node{ session.unacked.extract(sequence) }; node)
		{
			auto renumbered{ session.send_sequence++ & 0xFFFFFFu };
			std::memcpy(node.mapped().datagram.data() + 1u, &renumbered, 3u);
			node.mapped().sent_ms = functions::get_time_ms();
			node.key() = renumbered;

			send_to(session.address, node.mapped().datagram);
			session.unacked.insert(std::move(node));
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::send_to(const sockaddr_in& address, const std::vector<std::uint8_t>& data)
	{
		sendto(handle, reinterpret_cast<const char*>(data.data()), static_cast<std::int32_t>(data.size()), 0, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::queue(structures::session_s& session)
	{
		if (session.alive && !session.queued)
		{
			session.queued = true;
			dirty.push_back(&session);
		}
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::flush()
	{
		for (auto session : dirty)
		{
			session->queued = false;

			if (session->alive)
			{
				send_acks(*session);
				send_datagram(*session);
			}
		}

		dirty.clear();
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::tick(std::uint64_t now)
	{
		for (auto& [key, session] : sessions)
		{
			if (session->alive && now - session->seen_ms > session_timeout_ms)
			{
				close(*session, "timed out");
			}

			else if (session->alive)
			{
				expired.clear();

				for (auto& [sequence, entry] : session->unacked)
				{
					if (now - entry.sent_ms > resend_ms)
					{
						expired.push_back(sequence);
					}
				}

				for (auto sequence : expired)
				{
					resend(*session, sequence);
				}
			}
		}

		std::erase_if(sessions, [](const auto& entry) { return !entry.second->alive && !entry.second->queued; });
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::close(structures::session_s& session, const char* reason)
	{
		if (session.alive)
		{
			message.clear();
			functions::write_u8(message, raknet_disconnect);
			send(session, message, raknet_reliability_reliable_ordered);
			send_datagram(session);
			session.alive = false;
			bedrock_play.leave(session);

			if (session.name.size())
			{
				utils::log.write(color_play, "[bedrock](leave) '%s'", session.name.c_str());
			}

			if (reason)
			{
				utils::log.write(color_warn, "[bedrock](drop) '%s' %s", functions::format_address(session.address).c_str(), reason);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t raknet_c::get_online() const
	{
		return static_cast<std::uint32_t>(std::count_if(sessions.begin(), sessions.end(), [](const auto& entry) { return (entry.second->alive && entry.second->playing); }));
	}
	/*
	//=====================================================================================
	*/
	void raknet_c::shutdown()
	{
		for (auto& [key, session] : sessions)
		{
			if (session->alive)
			{
				bedrock_packet.kick(*session, "server closed");
				close(*session, nullptr);
			}
		}

		closesocket(handle);
	}
}

//=====================================================================================
