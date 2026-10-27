
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	java_protocol_c java_protocol;

	void java_protocol_c::handle(structures::client_s& client, structures::reader_s& frame)
	{
		if (auto id{ functions::read_varint(frame) }; frame.good())
		{
			switch (client.state)
			{
			case structures::state_handshake:
				handle_handshake(client, id, frame);
				break;

			case structures::state_status:
				handle_status(client, id, frame);
				break;

			case structures::state_login:
				handle_login(client, id, frame);
				break;

			case structures::state_configuration:
				handle_configuration(client, id, frame);
				break;

			case structures::state_play:
				java_play.handle(client, id, frame);
				break;
			}
		}

		else
		{
			network.close(client, "malformed packet id");
		}
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::handle_legacy(structures::client_s& client)
	{
		std::array<char, 256u> text{};
		auto length{ std::snprintf(text.data(), text.size(), "\xA7" "1%c%u%c%s%c%s%c%u%c%u", '\0', legacy_ping_protocol, '\0', server_versions, '\0', config.motd.c_str(), '\0', network.get_online(), '\0', config.max_players) };

		packet.clear();
		functions::write_u8(packet, legacy_kick_byte);
		functions::write_u16(packet, static_cast<std::uint16_t>(length));

		for (auto index{ 0 }; index < length; ++index)
		{
			functions::write_u16(packet, static_cast<std::uint8_t>(text[static_cast<std::size_t>(index)]));
		}

		network.send_raw(client, packet.data(), packet.size());
		network.finish(client);

		utils::log.write(color_status, "[status](legacy) ping from '%s'", client.address.c_str());
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::kick(structures::client_s& client, std::string_view reason)
	{
		packet.clear();

		switch (client.state)
		{
		case structures::state_login:
			functions::write_varint(packet, login_out_disconnect);
			functions::write_json_text(packet, reason);
			break;

		case structures::state_configuration:
			functions::write_varint(packet, client.layout.config_out_disconnect);
			functions::write_nbt_text(packet, reason);
			break;

		case structures::state_play:
			if (functions::is_modern(client.family))
			{
				functions::write_varint(packet, client.layout.play_out_disconnect);
				functions::write_nbt_text(packet, reason);
			}

			else
			{
				functions::write_varint(packet, v1_8_play_out_disconnect);
				functions::write_json_text(packet, reason);
			}

			break;
		}

		if (packet.size())
		{
			network.send(client, packet);
		}

		network.finish(client);

		utils::log.write(color_error, "[kick](%s) '%s' %.*s", functions::get_version_name(client.protocol), client.named() ? client.name.c_str() : client.address.c_str(), static_cast<std::int32_t>(reason.size()), reason.data());
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::handle_handshake(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		auto version{ functions::read_varint(in) };
		functions::read_string(in, max_address_length);
		functions::read_u16(in);
		auto next{ functions::read_varint(in) };

		if (id == handshake_in_intention && in.good() && next >= handshake_next_status && next <= handshake_next_transfer)
		{
			client.protocol = version;
			client.family = functions::get_family(version);
			client.layout = functions::get_layout(client.family);
			client.state = (next == handshake_next_status) ? structures::state_status : structures::state_login;
		}

		else
		{
			network.close(client, "malformed handshake");
		}
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::handle_status(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		if (id == status_in_request)
		{
			auto supported{ client.family != structures::family_none };

			std::array<char, 512u> json{};
			std::snprintf(json.data(), json.size(), "{\"version\":{\"name\":\"%s\",\"protocol\":%d},\"players\":{\"max\":%u,\"online\":%u,\"sample\":[]},\"description\":{\"text\":\"%s\"},\"enforcesSecureChat\":false}", supported ? functions::get_version_name(client.protocol) : server_versions, supported ? client.protocol : protocol_26_2, config.max_players, network.get_online(), config.motd.c_str());

			packet.clear();
			functions::write_varint(packet, status_out_response);
			functions::write_string(packet, json.data());
			network.send(client, packet);

			utils::log.write(color_status, "[status](ping) '%s' protocol %d from '%s'", functions::get_version_name(client.protocol), client.protocol, client.address.c_str());
		}

		else if (auto token{ functions::read_i64(in) }; id == status_in_ping && in.good())
		{
			packet.clear();
			functions::write_varint(packet, status_out_pong);
			functions::write_i64(packet, token);
			network.send(client, packet);

			network.finish(client);
		}

		else
		{
			network.close(client, "unexpected status packet");
		}
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::handle_login(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		if (id == login_in_start && !client.named())
		{
			start_login(client, in);
		}

		else if (id == login_in_acknowledged && client.named() && functions::is_modern(client.family))
		{
			client.state = structures::state_configuration;
			network.send_shared(client, java_registry.get(client.family).config);

			utils::log.write(color_config, "[config](begin) '%s' sent %.1f kb registry", client.name.c_str(), static_cast<std::double_t>(java_registry.get(client.family).config.size()) / 1024.0);
		}

		else
		{
			network.close(client, "unexpected login packet");
		}
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::handle_configuration(structures::client_s& client, std::int32_t id, structures::reader_s& in)
	{
		if (id == client.layout.config_in_finish)
		{
			client.state = structures::state_play;
			java_play.join(client);
		}
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::start_login(structures::client_s& client, structures::reader_s& in)
	{
		if (auto name{ functions::read_string(in, max_name_length) }; in.good() && functions::is_valid_name(name))
		{
			client.name = name;

			if (client.family == structures::family_none)
			{
				kick(client, std::string{ "you joined on " }.append(functions::get_version_name(client.protocol)).append(" (protocol ").append(std::to_string(client.protocol)).append(") - this server runs ").append(server_versions).append(" right now"));
			}

			else if (network.is_name_taken(client))
			{
				kick(client, "that name is already online");
			}

			else if (network.get_online() >= config.max_players)
			{
				kick(client, "server is full");
			}

			else
			{
				functions::make_offline_uuid(name, client.uuid_high, client.uuid_low);

				packet.clear();
				functions::write_varint(packet, login_out_compression);
				functions::write_varint(packet, static_cast<std::int32_t>(java_compression_threshold));
				network.send(client, packet);
				client.compressed = true;

				send_login_success(client);

				utils::log.write(color_login, "[login](%s) '%s' from '%s' uuid %s", functions::get_version_name(client.protocol), client.name.c_str(), client.address.c_str(), functions::format_uuid(client.uuid_high, client.uuid_low, true).c_str());

				if (functions::skips_configuration(client.protocol))
				{
					client.state = structures::state_play;
					java_play.join(client);
				}
			}
		}

		else
		{
			kick(client, "invalid username");
		}
	}
	/*
	//=====================================================================================
	*/
	void java_protocol_c::send_login_success(structures::client_s& client)
	{
		packet.clear();
		functions::write_varint(packet, login_out_success);

		switch (functions::get_login_format(client.protocol))
		{
		case structures::login_format_undashed:
			functions::write_string(packet, functions::format_uuid(client.uuid_high, client.uuid_low, false));
			functions::write_string(packet, client.name);
			break;

		case structures::login_format_dashed:
			functions::write_string(packet, functions::format_uuid(client.uuid_high, client.uuid_low, true));
			functions::write_string(packet, client.name);
			break;

		case structures::login_format_binary:
			functions::write_u64(packet, client.uuid_high);
			functions::write_u64(packet, client.uuid_low);
			functions::write_string(packet, client.name);
			break;

		case structures::login_format_properties:
		case structures::login_format_modern:
			functions::write_u64(packet, client.uuid_high);
			functions::write_u64(packet, client.uuid_low);
			functions::write_string(packet, client.name);
			functions::write_varint(packet, 0);
			break;

		case structures::login_format_session:
			functions::write_u64(packet, client.uuid_high);
			functions::write_u64(packet, client.uuid_low);
			functions::write_string(packet, client.name);
			functions::write_varint(packet, 0);
			functions::write_u64(packet, functions::mix_bits(client.uuid_high ^ client.connected_ms));
			functions::write_u64(packet, functions::mix_bits(client.uuid_low ^ client.connected_ms));
			break;

		case structures::login_format_strict:
			functions::write_u64(packet, client.uuid_high);
			functions::write_u64(packet, client.uuid_low);
			functions::write_string(packet, client.name);
			functions::write_varint(packet, 0);
			functions::write_bool(packet, true);
			break;
		}

		network.send(client, packet);
	}
}

//=====================================================================================
