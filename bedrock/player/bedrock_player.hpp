
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_player_c
	{
	public:

		void build();
		void spawn(structures::session_s& viewer, const structures::player_s& subject, bool entity);
		void move(structures::session_s& viewer, const structures::player_s& subject);
		void despawn(structures::session_s& viewer, const structures::player_s& subject);
		void send_actor_data(structures::session_s& session);
		void set_gamemode(structures::session_s& session, std::uint8_t mode);
		void look(structures::session_s& viewer, const structures::player_s& subject, std::float_t yaw, std::float_t pitch);

	private:

		static void write_actor_data(std::vector<std::uint8_t>& out, std::string_view name, bool self);
		static void write_skin(std::vector<std::uint8_t>& out, const structures::skin_s& skin);

		std::vector<std::uint8_t> body;
		std::shared_ptr<const structures::skin_s> default_skin;
	};

	extern bedrock_player_c bedrock_player;
}

//=====================================================================================
