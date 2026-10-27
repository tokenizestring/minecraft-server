
//=====================================================================================

#include "../../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	vitals_c vitals;

	void vitals_c::tick()
	{
		for (auto player : world.get_players())
		{
			if (is_live(*player))
			{
				update(*player);
			}

			if (player->vitals_dirty && player->edition == structures::edition_java)
			{
				player->vitals_dirty = false;
				java_play.send_health(*static_cast<structures::client_s*>(player->link));
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::reset(structures::player_s& player) const
	{
		player.health = max_health;
		player.food = max_food;
		player.saturation = start_saturation;
		player.exhaustion = 0.0f;
		player.fall = 0.0;
		player.regen_timer = 0u;
		player.starve_timer = 0u;
		player.eating = 0u;
		player.vitals_dirty = true;
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::moved(structures::player_s& player, std::double_t x, std::double_t y, std::double_t z, bool on_ground) const
	{
		auto gap_x{ x - player.x };
		auto gap_z{ z - player.z };
		auto drop{ player.y - y };
		auto flat{ std::sqrt(gap_x * gap_x + gap_z * gap_z) };
		auto falling{ drop > 0.0 || player.fall > 0.0 };
		auto kind{ falling ? lookup.get(structures::world_smp, static_cast<std::int32_t>(std::floor(x)), static_cast<std::int32_t>(std::floor(y)), static_cast<std::int32_t>(std::floor(z))) : kind_none };
		auto wet{ kind != kind_none && kind < placed_kind_base && kind != structures::block_air && blocks.is_fluid(static_cast<std::uint8_t>(kind)) };

		if (is_live(player))
		{
			player.exhaustion += flat < 10.0 ? static_cast<std::float_t>(flat) * exhaustion_walk : 0.0f;
			player.exhaustion += player.on_ground && on_ground == false && drop < 0.0 ? exhaustion_jump : 0.0f;
			player.fall = wet ? 0.0 : (drop > 0.0 ? player.fall + drop : player.fall);
		}

		if (auto hit{ static_cast<std::float_t>(std::ceil(player.fall - safe_fall)) }; is_live(player) && on_ground && player.fall > safe_fall)
		{
			java_play.send_sound(*static_cast<structures::client_s*>(player.link), hit > big_fall_damage ? sound_player_big_fall : sound_player_small_fall);
			damage(player, hit);
		}

		player.fall = on_ground ? 0.0 : player.fall;
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::damage(structures::player_s& player, std::float_t amount) const
	{
		if (is_live(player) && player.health > 0.0f)
		{
			player.health -= amount;
			player.exhaustion += exhaustion_hurt;
			player.vitals_dirty = true;
			java_play.send_sound(*static_cast<structures::client_s*>(player.link), sound_player_hurt);
		}

		if (is_live(player) && player.health <= 0.0f)
		{
			die(player);
		}
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::exhaust(structures::player_s& player, std::float_t amount) const
	{
		if (is_live(player))
		{
			player.exhaustion += amount;
		}
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::start_eating(structures::player_s& player) const
	{
		const auto& held{ player.inventory[player.held_slot] };
		auto food{ held.count ? items.get_food(held.item) : -1 };
		auto always{ food >= 0 && std::string_view{ food_names[food] }.find("golden_apple") != std::string_view::npos };

		if (is_live(player) && food >= 0 && (player.food < max_food || always))
		{
			player.eating = eat_ticks;
			player.eating_slot = player.held_slot;
		}
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::stop_eating(structures::player_s& player) const
	{
		player.eating = 0u;
	}
	/*
	//=====================================================================================
	*/
	bool vitals_c::is_live(const structures::player_s& player)
	{
		return player.edition == structures::edition_java && player.world == structures::world_smp && player.gamemode != gamemode_creative && player.gamemode != gamemode_spectator;
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::update(structures::player_s& player) const
	{
		const auto& held{ player.inventory[player.held_slot] };
		auto holding{ held.count && player.held_slot == player.eating_slot && items.get_food(held.item) >= 0 };
		auto regen{ player.food >= regen_food && player.health < max_health };
		auto fast{ player.food >= max_food && player.saturation > 0.0f };
		auto chewing{ player.eating != 0u };

		player.eating = holding && chewing ? player.eating - 1u : 0u;

		if (chewing && holding && player.eating == 0u)
		{
			finish_eating(player);
		}

		while (player.exhaustion >= exhaustion_step)
		{
			player.exhaustion -= exhaustion_step;
			player.food = player.saturation > 0.0f ? player.food : std::max(player.food - 1, 0);
			player.saturation = std::max(player.saturation - 1.0f, 0.0f);
			player.vitals_dirty = true;
		}

		player.regen_timer = regen ? player.regen_timer + 1u : 0u;
		player.starve_timer = player.food == 0 ? player.starve_timer + 1u : 0u;

		if (regen && player.regen_timer >= (fast ? fast_regen_ticks : regen_ticks))
		{
			player.health = std::min(player.health + 1.0f, max_health);
			player.exhaustion += exhaustion_heal;
			player.regen_timer = 0u;
			player.vitals_dirty = true;
		}

		if (player.starve_timer >= starve_ticks && player.health > starve_floor)
		{
			player.starve_timer = 0u;
			damage(player, 1.0f);
		}
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::finish_eating(structures::player_s& player) const
	{
		auto& held{ player.inventory[player.held_slot] };
		auto food{ items.get_food(held.item) };
		auto nutrition{ food_nutrition[food] };

		player.food = std::min(player.food + static_cast<std::int32_t>(nutrition), max_food);
		player.saturation = std::min(player.saturation + static_cast<std::float_t>(nutrition) * food_saturation[food] * 2.0f, static_cast<std::float_t>(player.food));
		player.vitals_dirty = true;
		held.count = static_cast<std::uint8_t>(held.count - 1u);
		held.item = held.count ? held.item : 0u;

		java_play.send_event(*static_cast<structures::client_s*>(player.link), entity_event_eaten);
		java_play.send_sound(*static_cast<structures::client_s*>(player.link), sound_player_burp);
		java_inventory.send_slot(*static_cast<structures::client_s*>(player.link), player.held_slot);
	}
	/*
	//=====================================================================================
	*/
	void vitals_c::die(structures::player_s& player) const
	{
		for (auto& stack : player.inventory)
		{
			if (stack.count && stack.item)
			{
				drops.spawn(structures::world_smp, stack, player.x, player.y + 1.0, player.z);
			}

			stack = {};
		}

		player.cursor = {};
		reset(player);
		chat.notify(player, "you died, your items are where you fell");
		utils::log.write(color_play, "[vitals](death) '%s' at %.0f %.0f %.0f", player.name.c_str(), player.x, player.y, player.z);
		java_play.switch_world(*static_cast<structures::client_s*>(player.link), structures::world_smp);
		profile.save(player);
	}
}

//=====================================================================================
