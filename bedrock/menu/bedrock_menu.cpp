
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_menu_c bedrock_menu;

	void bedrock_menu_c::build()
	{
		for (const auto& entry : hub.get_menus())
		{
			std::string form{ "{\"type\":\"form\",\"title\":" };
			functions::append_json_string(form, entry.title);
			form.append(",\"content\":\"\",\"buttons\":[");

			for (const auto& item : entry.items)
			{
				form.append(&item == &entry.items.front() ? "{\"text\":" : ",{\"text\":");
				functions::append_json_string(form, item.lore.size() ? std::string{ item.name }.append("\n").append(item.lore.front()) : item.name);
				form.append("}");
			}

			forms.push_back(form.append("]}"));
		}

		utils::log.write(color_play, "[hub](build) %zu bedrock forms", forms.size());
	}
	/*
	//=====================================================================================
	*/
	void bedrock_menu_c::open(structures::session_s& session, std::uint32_t index)
	{
		send(session, index + 1u, forms[index]);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_menu_c::open_social(structures::session_s& session)
	{
		auto entries{ friends.list(session.player) };
		std::string form{ "{\"type\":\"form\",\"title\":\"friends\",\"content\":" };
		session.social_view.clear();

		functions::append_json_string(form, entries.size() ? "tap a player to add them, tap a request to accept, tap a friend to remove them" : "nobody else is online yet");
		form.append(",\"buttons\":[");

		for (const auto& entry : entries)
		{
			if (session.social_view.size() < bedrock_market_buttons)
			{
				form.append(session.social_view.size() ? ",{\"text\":" : "{\"text\":");
				functions::append_json_string(form, entry.name + "\n" + social_labels[entry.state]);
				form.append("}");
				session.social_view.push_back((entry.state == social_friend_online || entry.state == social_friend_offline ? "-" : "+") + entry.name);
			}
		}

		send(session, bedrock_social_form, form.append("]}"));
	}
	/*
	//=====================================================================================
	*/
	void bedrock_menu_c::handle_response(structures::session_s& session, structures::reader_s& in)
	{
		auto form{ static_cast<std::uint32_t>(functions::read_varint(in)) };
		auto answered{ functions::read_u8(in) };
		auto data{ answered ? functions::read_string(in, max_chat_length) : std::string_view{} };
		auto start{ data.find_first_of("0123456789") };
		auto button{ std::numeric_limits<std::uint32_t>::max() };

		if (start != std::string_view::npos)
		{
			std::from_chars(data.data() + start, data.data() + data.size(), button);
		}

		if (in.good() && session.playing && answered && form == bedrock_market_buy_form)
		{
			bedrock_market.choose(session, button);
		}

		else if (in.good() && session.playing && answered && form == bedrock_market_sell_form)
		{
			bedrock_market.sell(session, data);
		}

		else if (in.good() && session.playing && form == bedrock_social_form && button < session.social_view.size())
		{
			const auto& entry{ session.social_view[button] };
			friends.click(session.player, std::string_view{ entry }.substr(1u), entry.front() == '-');
			session.social_view.clear();
		}

		else if (in.good() && session.playing && form == session.player.menu)
		{
			hub.choose(session.player, button);
			session.player.menu = 0u;
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_menu_c::send(structures::session_s& session, std::uint32_t id, std::string_view form)
	{
		body.clear();
		functions::write_varint(body, static_cast<std::int32_t>(id));
		functions::write_string(body, form);
		bedrock_packet.send(session, bedrock_modal_form_request, body);
	}
}

//=====================================================================================
