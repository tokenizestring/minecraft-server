
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	profile_c profile;

	void profile_c::build()
	{
		CreateDirectoryA(functions::get_local_path(profile_folder).c_str(), nullptr);
		last_save = functions::get_time_ms();
	}
	/*
	//=====================================================================================
	*/
	void profile_c::load(structures::player_s& player) const
	{
		player.balance = static_cast<std::int64_t>(config.start_balance);
		player.inventory = {};
		player.cursor = {};

		if (auto file{ std::fopen(get_path(player.name).c_str(), "rb") }; file)
		{
			std::array<char, 256u> line{};
			std::array<char, 128u> name{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::int64_t balance{};
				std::uint32_t slot{};
				std::uint32_t count{};

				if (std::sscanf(line.data(), "balance %lld", &balance) == 1)
				{
					player.balance = balance;
				}

				else if (std::sscanf(line.data(), "slot %u %127s %u", &slot, name.data(), &count) == 3 && slot < inventory_size && count)
				{
					auto item{ items.find_name(name.data()) };
					player.inventory[slot] = { item, static_cast<std::uint8_t>(item ? std::min(count, 255u) : 0u) };
				}
			}

			std::fclose(file);
		}
	}
	/*
	//=====================================================================================
	*/
	void profile_c::save(const structures::player_s& player) const
	{
		if (auto file{ std::fopen(get_path(player.name).c_str(), "wb") }; file)
		{
			std::fprintf(file, "balance %lld\n", player.balance);

			for (auto slot{ 0u }; slot < inventory_size; ++slot)
			{
				if (player.inventory[slot].count)
				{
					std::fprintf(file, "slot %u %s %u\n", slot, items.get_name(player.inventory[slot].item).c_str(), static_cast<std::uint32_t>(player.inventory[slot].count));
				}
			}

			std::fclose(file);
		}
	}
	/*
	//=====================================================================================
	*/
	void profile_c::credit(std::string_view name, std::int64_t amount) const
	{
		if (auto* online{ find_online(name) }; online)
		{
			online->balance += amount;
		}

		else
		{
			structures::player_s offline{};
			offline.name.assign(name);
			load(offline);
			offline.balance += amount;
			save(offline);
		}
	}
	/*
	//=====================================================================================
	*/
	void profile_c::tick(std::uint64_t now)
	{
		if (now - last_save >= autosave_ms)
		{
			save_all();
			last_save = now;
		}
	}
	/*
	//=====================================================================================
	*/
	void profile_c::save_all() const
	{
		for (auto player : world.get_players())
		{
			if (player->npc == false)
			{
				save(*player);
			}
		}
	}
	/*
	//=====================================================================================
	*/
	structures::player_s* profile_c::find_online(std::string_view name)
	{
		structures::player_s* found{ nullptr };

		for (auto player : world.get_players())
		{
			found = found == nullptr && !player->npc && player->name == name ? player : found;
		}

		return found;
	}
	/*
	//=====================================================================================
	*/
	std::string profile_c::get_path(std::string_view name)
	{
		std::string file{ profile_folder };
		file.push_back('/');

		for (auto character : name)
		{
			auto lower{ static_cast<char>(std::tolower(static_cast<std::uint8_t>(character))) };
			file.push_back((lower >= 'a' && lower <= 'z') || (lower >= '0' && lower <= '9') ? lower : '_');
		}

		return functions::get_local_path(file.append(".txt").c_str());
	}
}

//=====================================================================================
