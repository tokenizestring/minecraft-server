
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_login_c bedrock_login;

	void bedrock_login_c::handle_request_settings(structures::session_s& session, structures::reader_s& in)
	{
		session.protocol = static_cast<std::int32_t>(functions::read_u32(in));

		if (in.good() && session.protocol == bedrock_protocol)
		{
			body.clear();
			functions::write_u16_le(body, bedrock_compression_threshold);
			functions::write_u16_le(body, bedrock_compression_zlib);
			functions::write_bool(body, false);
			functions::write_u8(body, 0u);
			functions::write_f32_le(body, 0.0f);
			bedrock_packet.send(session, bedrock_network_settings, body);

			session.compression = true;
		}

		else if (in.good())
		{
			bedrock_packet.send_play_status(session, session.protocol < bedrock_protocol ? bedrock_status_failed_client : bedrock_status_failed_server);
			utils::log.write(color_error, "[kick](bedrock) '%s' protocol %d, server runs %d '%s'", functions::format_address(session.address).c_str(), session.protocol, bedrock_protocol, bedrock_version);
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_login_c::handle_login(structures::session_s& session, structures::reader_s& in)
	{
		auto protocol{ static_cast<std::int32_t>(functions::read_u32(in)) };
		auto request{ functions::read_string(in, max_batch_size / 4u) };

		if (in.good() && session.compression)
		{
			structures::reader_s request_in{ reinterpret_cast<const std::uint8_t*>(request.data()), request.size(), 0u, true };

			if (auto chain_length{ functions::read_u32_le(request_in) }; request_in.good() && request_in.remaining() >= chain_length)
			{
				read_identity(session, request.substr(request_in.cursor, chain_length));
				request_in.cursor += chain_length;
			}

			if (auto client_length{ functions::read_u32_le(request_in) }; request_in.good() && request_in.remaining() >= client_length)
			{
				read_identity(session, request.substr(request_in.cursor, client_length));
			}

			utils::log.write(color_login, "[login](bedrock) '%s' xuid '%s' protocol %d from '%s' %.1f kb login", session.name.size() ? session.name.c_str() : "unknown", session.xuid.c_str(), protocol, functions::format_address(session.address).c_str(), static_cast<std::double_t>(request.size()) / 1024.0);

			if (session.name.size() && network.get_online() + raknet.get_online() < config.max_players)
			{
				session.entity_id = world.allocate_entity_id();
				bedrock_packet.send_play_status(session, bedrock_status_login_success);
				send_resource_packs_info(session);
			}

			else if (session.name.size())
			{
				bedrock_packet.kick(session, "server is full");
			}

			else
			{
				bedrock_packet.kick(session, "could not read your profile from the login");
			}
		}

		else
		{
			raknet.close(session, "malformed login");
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_login_c::read_identity(structures::session_s& session, std::string_view text)
	{
		auto at{ text.find("eyJ") };

		while (at != std::string_view::npos)
		{
			auto end{ text.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.", at) };
			auto token{ text.substr(at, end - at) };
			auto first{ token.find('.') };

			if (first != std::string_view::npos && token.find('.', first + 1u) != std::string_view::npos)
			{
				auto payload{ functions::decode_base64(token.substr(first + 1u, token.find('.', first + 1u) - first - 1u)) };

				if (auto name{ functions::find_json_string(payload, "displayName") }; name.size() && session.name.size() < 1u)
				{
					session.name = name;
				}

				if (auto name{ functions::find_json_string(payload, "ThirdPartyName") }; name.size() && session.name.size() < 1u)
				{
					session.name = name;
				}

				if (auto name{ functions::find_json_string(payload, "xname") }; name.size() && session.name.size() < 1u)
				{
					session.name = name;
				}

				if (auto xuid{ functions::find_json_string(payload, "XUID") }; xuid.size() && session.xuid.size() < 1u)
				{
					session.xuid = xuid;
				}

				if (auto xuid{ functions::find_json_string(payload, "xid") }; xuid.size() && session.xuid.size() < 1u)
				{
					session.xuid = xuid;
				}

				if (auto image{ functions::find_json_string(payload, "SkinData") }; image.size())
				{
					auto skin{ std::make_shared<structures::skin_s>() };
					skin->id = functions::find_json_string(payload, "SkinId");
					skin->patch = functions::decode_base64(functions::find_json_string(payload, "SkinResourcePatch"));
					skin->geometry = functions::decode_base64(functions::find_json_string(payload, "SkinGeometryData"));
					skin->engine = functions::decode_base64(functions::find_json_string(payload, "SkinGeometryDataEngineVersion"));
					skin->width = functions::find_json_number(payload, "SkinImageWidth");
					skin->height = functions::find_json_number(payload, "SkinImageHeight");
					skin->arm = (functions::find_json_string(payload, "ArmSize") == "slim") ? 0u : bedrock_arm_wide;
					skin->premium = (functions::find_json_literal(payload, "PremiumSkin") == "true");

					auto decoded{ functions::decode_base64(image) };
					skin->image.assign(decoded.begin(), decoded.end());

					if (skin->image.size() == static_cast<std::size_t>(skin->width) * skin->height * 4u)
					{
						session.player.skin = std::move(skin);
						session.player.platform = static_cast<std::int32_t>(functions::find_json_number(payload, "DeviceOS"));
					}
				}
			}

			at = (end == std::string_view::npos) ? end : text.find("eyJ", end);
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_login_c::handle_pack_response(structures::session_s& session, structures::reader_s& in)
	{
		auto response{ static_cast<std::uint32_t>(functions::read_varint(in)) };
		functions::read_string(in, max_name_length * 2u);

		if (in.good() && response == bedrock_pack_have_all)
		{
			send_resource_pack_stack(session);
		}

		else if (in.good() && response == bedrock_pack_completed)
		{
			bedrock_play.start_game(session);
		}

		else if (in.good())
		{
			bedrock_packet.kick(session, "resource packs refused");
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_login_c::send_resource_packs_info(structures::session_s& session)
	{
		body.clear();
		functions::write_bool(body, false);
		functions::write_bool(body, false);
		functions::write_bool(body, false);
		functions::write_bool(body, false);
		functions::write_u64_le(body, 0ui64);
		functions::write_u64_le(body, 0ui64);
		functions::write_string(body, bedrock_template_version);
		functions::write_varint(body, 0);
		bedrock_packet.send(session, bedrock_resource_packs_info, body);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_login_c::send_resource_pack_stack(structures::session_s& session)
	{
		body.clear();
		functions::write_bool(body, false);
		functions::write_varint(body, 0);
		functions::write_string(body, bedrock_any_version);
		functions::write_u32_le(body, 0u);
		functions::write_bool(body, false);
		functions::write_bool(body, false);
		bedrock_packet.send(session, bedrock_resource_pack_stack, body);
	}
}

//=====================================================================================
