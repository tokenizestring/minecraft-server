
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_menu_c bedrock_menu;

	void bedrock_menu_c::build()
	{
		for (const auto& entry : npc.get_npcs())
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

		utils::log.write(color_play, "[npc](build) %zu bedrock forms", forms.size());
	}
	/*
	//=====================================================================================
	*/
	void bedrock_menu_c::open(structures::session_s& session, std::uint32_t index)
	{
		body.clear();
		functions::write_varint(body, static_cast<std::int32_t>(index + 1u));
		functions::write_string(body, forms[index]);
		bedrock_packet.send(session, bedrock_modal_form_request, body);
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

		else if (in.good() && session.playing && form == session.player.menu)
		{
			npc.choose(session.player, button);
			session.player.menu = 0u;
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_menu_c::handle_transaction(structures::session_s& session, structures::reader_s& in)
	{
		if (auto request{ functions::read_zigzag32(in) }; request && in.good())
		{
			auto containers{ static_cast<std::uint32_t>(functions::read_varint(in)) };

			for (auto index{ 0u }; index < containers && in.good(); ++index)
			{
				functions::read_u8(in);
				auto slots{ static_cast<std::uint32_t>(functions::read_varint(in)) };
				in.valid = in.good() && in.remaining() >= slots;
				in.cursor += in.good() ? slots : 0u;
			}
		}

		auto type{ static_cast<std::uint32_t>(functions::read_varint(in)) };
		auto actions{ static_cast<std::uint32_t>(functions::read_varint(in)) };
		auto target{ functions::read_varint(in) };

		if (in.good() && session.playing && type == bedrock_transaction_use_on_actor && actions == 0u)
		{
			npc.interact(session.player, target);
		}
	}
}

//=====================================================================================
