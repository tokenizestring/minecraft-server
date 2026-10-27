
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	market_c market;

	void market_c::load()
	{
		next_id = 1u;

		if (auto file{ std::fopen(functions::get_local_path(market_path).c_str(), "rb") }; file)
		{
			std::array<char, 512u> line{};
			std::array<char, 128u> seller{};
			std::array<char, 128u> name{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::uint32_t id{};
				std::uint32_t count{};
				std::int64_t price{};

				if (std::sscanf(line.data(), "%u\t%127[^\t]\t%127s\t%u\t%lld", &id, seller.data(), name.data(), &count, &price) == 5 && items.find_name(name.data()) && count)
				{
					listings.push_back(structures::listing_s{ id, seller.data(), { items.find_name(name.data()), static_cast<std::uint8_t>(std::min(count, 255u)) }, price });
					next_id = std::max(next_id, id + 1u);
				}
			}

			std::fclose(file);
		}

		utils::log.write(color_play, "[market](load) %zu listings", listings.size());
	}
	/*
	//=====================================================================================
	*/
	void market_c::sell(structures::player_s& player, std::string_view argument)
	{
		std::uint64_t price{};
		auto [end, error]{ std::from_chars(argument.data(), argument.data() + argument.size(), price) };
		auto& stack{ player.inventory[player.held_slot] };
		auto owned{ std::count_if(listings.begin(), listings.end(), [&player](const auto& listing) { return listing.seller == player.name; }) };

		if (error != std::errc{} || price == 0u || price > config.max_price)
		{
			chat.notify(player, "use /sell <price> while holding the item");
		}

		else if (stack.count == 0u)
		{
			chat.notify(player, "hold the item you want to sell");
		}

		else if (owned >= static_cast<std::ptrdiff_t>(market_listings_per_player))
		{
			chat.notify(player, "you have too many listings, take some back in /buy");
		}

		else
		{
			listings.push_back(structures::listing_s{ next_id, player.name, stack, static_cast<std::int64_t>(price) });
			next_id += 1u;
			chat.notify(player, "listed " + std::to_string(stack.count) + " " + items.get_label(stack.item) + " for " + std::to_string(price));
			utils::log.write(color_play, "[market](sell) '%s' %u %s for %llu", player.name.c_str(), static_cast<std::uint32_t>(stack.count), items.get_name(stack.item).c_str(), price);
			stack = {};
			save();
			profile.save(player);
		}
	}
	/*
	//=====================================================================================
	*/
	void market_c::buy(structures::player_s& player, std::uint32_t id)
	{
		auto found{ std::find_if(listings.begin(), listings.end(), [id](const auto& listing) { return listing.id == id; }) };
		auto exists{ found != listings.end() };
		auto own{ exists && found->seller == player.name };
		auto room{ exists && has_room(player, found->stack) };

		if (room && own)
		{
			inventory.add(player, found->stack.item, found->stack.count);
			chat.notify(player, "took back " + items.get_label(found->stack.item));
			listings.erase(found);
			save();
			profile.save(player);
		}

		else if (room && player.balance < found->price)
		{
			chat.notify(player, "you need " + std::to_string(found->price) + ", you have " + std::to_string(player.balance));
		}

		else if (exists && !room)
		{
			chat.notify(player, "your inventory is full");
		}

		else if (found == listings.end())
		{
			chat.notify(player, "that listing is gone");
		}

		else
		{
			auto listing{ *found };
			listings.erase(found);
			player.balance -= listing.price;
			inventory.add(player, listing.stack.item, listing.stack.count);
			profile.credit(listing.seller, listing.price);
			save();
			profile.save(player);
			chat.notify(player, "bought " + std::to_string(listing.stack.count) + " " + items.get_label(listing.stack.item) + " for " + std::to_string(listing.price));
			utils::log.write(color_play, "[market](buy) '%s' bought %u %s from '%s' for %lld", player.name.c_str(), static_cast<std::uint32_t>(listing.stack.count), items.get_name(listing.stack.item).c_str(), listing.seller.c_str(), listing.price);

			if (auto* seller{ profile_c::find_online(listing.seller) }; seller)
			{
				chat.notify(*seller, player.name + " bought your " + items.get_label(listing.stack.item) + " for " + std::to_string(listing.price));
			}
		}
	}
	/*
	//=====================================================================================
	*/
	std::vector<const structures::listing_s*> market_c::filter(std::string_view text) const
	{
		std::vector<const structures::listing_s*> out{};

		for (const auto& listing : listings)
		{
			if (text.empty() || items.matches(listing.stack.item, text))
			{
				out.push_back(&listing);
			}
		}

		return out;
	}
	/*
	//=====================================================================================
	*/
	const structures::listing_s* market_c::find(std::uint32_t id) const
	{
		auto found{ std::find_if(listings.begin(), listings.end(), [id](const auto& listing) { return listing.id == id; }) };
		return found != listings.end() ? &*found : nullptr;
	}
	/*
	//=====================================================================================
	*/
	void market_c::save() const
	{
		if (auto file{ std::fopen(functions::get_local_path(market_path).c_str(), "wb") }; file)
		{
			for (const auto& listing : listings)
			{
				std::fprintf(file, "%u\t%s\t%s\t%u\t%lld\n", listing.id, listing.seller.c_str(), items.get_name(listing.stack.item).c_str(), static_cast<std::uint32_t>(listing.stack.count), listing.price);
			}

			std::fclose(file);
		}
	}
	/*
	//=====================================================================================
	*/
	bool market_c::has_room(const structures::player_s& player, const structures::item_stack_s& stack)
	{
		structures::player_s copy{};
		copy.inventory = player.inventory;
		return inventory.add(copy, stack.item, stack.count) == 0u;
	}
}

//=====================================================================================
