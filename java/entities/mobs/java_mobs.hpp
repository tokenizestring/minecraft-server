
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_mobs_c
	{
	public:

		void spawn(structures::client_s& viewer, const structures::mob_s& mob);
		void move(structures::client_s& viewer, const structures::mob_s& mob, bool moved, bool turned);
		void hurt(structures::client_s& viewer, const structures::mob_s& mob);
		void die(structures::client_s& viewer, const structures::mob_s& mob);
		void sound(structures::client_s& viewer, const structures::mob_s& mob, std::uint32_t which);
		void remove(structures::client_s& viewer, std::int32_t entity_id);

	private:

		void write_teleport(const structures::client_s& viewer, const structures::mob_s& mob);

		static std::int64_t get_step(std::double_t from, std::double_t to, std::double_t scale);

		std::vector<std::uint8_t> packet;
	};

	extern java_mobs_c java_mobs;
}

//=====================================================================================
