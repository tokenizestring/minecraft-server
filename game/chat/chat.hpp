
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class chat_c
	{
	public:

		void join(structures::player_s& player);
		void say(const structures::player_s& author, std::string_view message);
		void tick(std::uint64_t now);
		void toggle(structures::player_s& player, std::string_view argument);
		void notify(structures::player_s& player, std::string_view message);

	private:

		void add_lines(std::string_view message, std::uint64_t now);
		void refresh();
		void open(structures::player_s& player);
		void show(structures::player_s& viewer);

		std::vector<std::string> lines;
		std::vector<std::uint64_t> times;
		std::string text;
		std::uint32_t next_color;
	};

	extern chat_c chat;
}

//=====================================================================================
