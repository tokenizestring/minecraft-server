
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class profile_c
	{
	public:

		void build();
		void load(structures::player_s& player) const;
		void save(const structures::player_s& player) const;
		void credit(std::string_view name, std::int64_t amount) const;

		template <typename change_t>
		void edit(std::string_view name, change_t change) const
		{
			structures::player_s offline{};
			offline.name.assign(name);
			load(offline);
			change(offline);
			save(offline);
		}
		void tick(std::uint64_t now);
		void save_all() const;

		static structures::player_s* find_online(std::string_view name);

	private:

		static std::string get_path(std::string_view name);

		std::uint64_t last_save;
	};

	extern profile_c profile;
}

//=====================================================================================
