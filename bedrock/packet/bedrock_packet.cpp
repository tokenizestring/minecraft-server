
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_packet_c bedrock_packet;

	void bedrock_packet_c::handle(structures::session_s& session, const std::uint8_t* data, std::size_t size)
	{
		structures::reader_s in{ data, size, 0u, true };

		if (session.compression && size && data[0] == bedrock_compression_zlib)
		{
			in.valid = zlib.inflate_raw(data + 1u, size - 1u, inflated, max_batch_size);
			in.data = inflated.data();
			in.size = inflated.size();
		}

		else if (session.compression && size && data[0] == bedrock_compression_none)
		{
			in = structures::reader_s{ data + 1u, size - 1u, 0u, true };
		}

		else if (session.compression)
		{
			in.valid = false;
		}

		auto malformed{ !in.good() };

		while (in.good() && in.remaining() && session.alive)
		{
			if (auto length{ static_cast<std::uint32_t>(functions::read_varint(in)) }; in.good() && length && in.remaining() >= length)
			{
				structures::reader_s packet_in{ in.data + in.cursor, length, 0u, true };
				in.cursor += length;

				auto header{ static_cast<std::uint32_t>(functions::read_varint(packet_in)) };
				handle_packet(session, header & 0x3FFu, packet_in);
			}

			else
			{
				malformed = true;
				in.valid = false;
			}
		}

		if (malformed)
		{
			raknet.close(session, "malformed batch");
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_packet_c::handle_packet(structures::session_s& session, std::uint32_t id, structures::reader_s& in)
	{
		switch (id)
		{
		case bedrock_request_network_settings:
			bedrock_login.handle_request_settings(session, in);
			break;

		case bedrock_login_request:
			bedrock_login.handle_login(session, in);
			break;

		case bedrock_resource_pack_response:
			bedrock_login.handle_pack_response(session, in);
			break;

		case bedrock_request_chunk_radius:
			bedrock_play.handle_chunk_radius(session, in);
			break;

		case bedrock_set_local_player_initialized:
			bedrock_play.join(session);
			break;

		case bedrock_player_auth_input:
			bedrock_play.handle_auth_input(session, in);
			break;

		case bedrock_subchunk_request:
			bedrock_chunk.handle_subchunk_request(session, in);
			break;

		case bedrock_text:
			bedrock_chat.handle(session, in);
			break;

		case bedrock_inventory_transaction:
			bedrock_menu.handle_transaction(session, in);
			break;

		case bedrock_modal_form_response:
			bedrock_menu.handle_response(session, in);
			break;

		case bedrock_command_request:
			bedrock_chat.handle_command(session, in);
			break;

		case bedrock_packet_violation:
			handle_violation(session, in);
			break;
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_packet_c::send(structures::session_s& session, std::uint32_t id, const std::vector<std::uint8_t>& payload)
	{
		packet.clear();
		functions::write_varint(packet, static_cast<std::int32_t>(id));
		functions::write_bytes(packet, payload.data(), payload.size());

		batch.clear();
		functions::write_u8(batch, raknet_game_packet);

		if (session.compression)
		{
			functions::write_u8(batch, bedrock_compression_none);
		}

		functions::write_varint(batch, static_cast<std::int32_t>(packet.size()));
		functions::write_bytes(batch, packet.data(), packet.size());

		raknet.send(session, batch, raknet_reliability_reliable_ordered);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_packet_c::send_play_status(structures::session_s& session, std::int32_t status)
	{
		body.clear();
		functions::write_i32(body, status);
		send(session, bedrock_play_status, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_packet_c::kick(structures::session_s& session, std::string_view reason)
	{
		body.clear();
		functions::write_zigzag32(body, bedrock_disconnect_reason_unknown);
		functions::write_varint(body, 0);
		functions::write_string(body, reason);
		functions::write_string(body, "");
		send(session, bedrock_disconnect, body);

		utils::log.write(color_error, "[kick](bedrock) '%s' %.*s", session.name.size() ? session.name.c_str() : functions::format_address(session.address).c_str(), static_cast<std::int32_t>(reason.size()), reason.data());
	}
	/*
	//=====================================================================================
	*/
	void bedrock_packet_c::handle_violation(structures::session_s& session, structures::reader_s& in)
	{
		auto type{ functions::read_zigzag32(in) };
		auto severity{ functions::read_zigzag32(in) };
		auto packet_id{ functions::read_zigzag32(in) };
		auto context{ functions::read_string(in, max_batch_size / 4u) };

		utils::log.write(color_error, "[bedrock](violation) '%s' packet %d type %d severity %d '%.*s'", session.name.c_str(), packet_id, type, severity, static_cast<std::int32_t>(context.size()), context.data());
	}
}

//=====================================================================================
