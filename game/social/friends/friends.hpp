
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class friends_c
	{
	public:

		void click(structures::player_s& player, std::string_view name, bool remove) const;
		void joined(structures::player_s& player) const;
		std::vector<structures::social_entry_s> list(const structures::player_s& player) const;

	private:

		void link(structures::player_s& player, std::string_view name) const;
		void unlink(structures::player_s& player, std::string_view name) const;

		static bool has(const std::vector<std::string>& names, std::string_view name);
	};

	extern friends_c friends;
}

//=====================================================================================
