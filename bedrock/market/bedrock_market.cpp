
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_market_c bedrock_market;

	void bedrock_market_c::open_buy(structures::session_s& session, std::string_view filter)
	{
		auto shown{ market.filter(filter) };
		auto count{ std::min(static_cast<std::uint32_t>(shown.size()), bedrock_market_buttons) };
		auto content{ "balance " + std::to_string(session.player.balance) + "\n" };
		session.market_view.clear();

		form.assign("{\"type\":\"form\",\"title\":");
		functions::append_json_string(form, filter.size() ? std::string{ market_title }.append(": ").append(filter) : std::string{ market_title });
		form.append(",\"content\":");
		functions::append_json_string(form, content.append(count ? "tap a listing to buy it, or your own to take it back" : "nothing for sale yet, list items with /sell"));
		form.append(",\"buttons\":[");

		for (auto index{ 0u }; index < count; ++index)
		{
			const auto& listing{ *shown[index] };
			auto text{ std::to_string(listing.stack.count) + "x " + items.get_label(listing.stack.item) + " - " + std::to_string(listing.price) + "\n" };
			form.append(index ? ",{\"text\":" : "{\"text\":");
			functions::append_json_string(form, text.append(listing.seller == session.player.name ? "yours, tap to take back" : listing.seller));
			form.append("}");
			session.market_view.push_back(listing.id);
		}

		form.append("]}");
		send(session, bedrock_market_buy_form);
	}
	/*
	//=====================================================================================
	*/
	void bedrock_market_c::open_sell(structures::session_s& session)
	{
		session.sell_slots.clear();

		form.assign("{\"type\":\"custom_form\",\"title\":\"sell\",\"content\":[{\"type\":\"dropdown\",\"text\":\"item\",\"options\":[");

		for (auto slot{ 0u }; slot < inventory_size; ++slot)
		{
			if (const auto& stack{ session.player.inventory[slot] }; stack.count && stack.item)
			{
				form.append(session.sell_slots.size() ? "," : "");
				functions::append_json_string(form, items.get_label(stack.item) + " x" + std::to_string(stack.count));
				session.sell_slots.push_back(slot);
			}
		}

		form.append("]},{\"type\":\"input\",\"text\":\"price\",\"placeholder\":\"price\"}]}");

		if (session.sell_slots.size())
		{
			send(session, bedrock_market_sell_form);
		}

		else
		{
			chat.notify(session.player, "you have nothing to sell, items you /buy can be sold again");
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_market_c::choose(structures::session_s& session, std::uint32_t button)
	{
		if (button < session.market_view.size())
		{
			market.buy(session.player, session.market_view[button]);
		}

		session.market_view.clear();
	}
	/*
	//=====================================================================================
	*/
	void bedrock_market_c::sell(structures::session_s& session, std::string_view data)
	{
		auto start{ data.find_first_of("0123456789") };
		auto open{ data.find('"') };
		auto close{ open != std::string_view::npos ? data.find('"', open + 1u) : std::string_view::npos };
		auto choice{ std::numeric_limits<std::uint32_t>::max() };

		if (start != std::string_view::npos)
		{
			std::from_chars(data.data() + start, data.data() + data.size(), choice);
		}

		if (choice < session.sell_slots.size() && close != std::string_view::npos)
		{
			market.sell(session.player, session.sell_slots[choice], data.substr(open + 1u, close - open - 1u));
		}

		else
		{
			chat.notify(session.player, "pick an item and type a price");
		}

		session.sell_slots.clear();
	}
	/*
	//=====================================================================================
	*/
	void bedrock_market_c::send(structures::session_s& session, std::uint32_t id)
	{
		body.clear();
		functions::write_varint(body, static_cast<std::int32_t>(id));
		functions::write_string(body, form);
		bedrock_packet.send(session, bedrock_modal_form_request, body);
	}
}

//=====================================================================================
