
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_play_c
	{
	public:

		void join(structures::client_s& client);
		void handle(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void tick(structures::client_s& client, std::uint64_t now);
		void leave(structures::client_s& client);
		void switch_world(structures::client_s& client, structures::world_e target);
		void switch_world(structures::client_s& client, structures::world_e target, const structures::spawn_s& spawn);
		void send_time(structures::client_s& client);

	private:

		void join_modern(structures::client_s& client);
		void join_1_8(structures::client_s& client);
		void handle_modern(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void handle_1_8(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void teleport(structures::client_s& client);
		void apply_movement(structures::client_s& client, std::double_t x, std::double_t y, std::double_t z, std::float_t yaw, std::float_t pitch, bool on_ground);
		void send_keep_alive(structures::client_s& client, std::uint64_t now);

		std::vector<std::uint8_t> packet;
	};

	extern java_play_c java_play;
}

//=====================================================================================
