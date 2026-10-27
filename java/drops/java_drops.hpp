
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_drops_c
	{
	public:

		void spawn(structures::client_s& viewer, const structures::drop_s& drop);
		void update(structures::client_s& viewer, const structures::drop_s& drop);
		void move(structures::client_s& viewer, const structures::drop_s& drop);
		void collect(structures::client_s& viewer, const structures::drop_s& drop, std::int32_t collector, std::uint32_t count);
		void remove(structures::client_s& viewer, std::int32_t entity_id);

	private:

		static std::int64_t get_step(std::double_t from, std::double_t to, std::double_t scale);

		std::vector<std::uint8_t> packet;
	};

	extern java_drops_c java_drops;
}

//=====================================================================================
