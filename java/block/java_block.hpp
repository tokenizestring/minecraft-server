
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_block_c
	{
	public:

		void handle_modern(structures::client_s& client, std::int32_t id, structures::reader_s& in);
		void handle_held(structures::client_s& client, structures::reader_s& in);
		void handle_creative(structures::client_s& client, structures::reader_s& in);
		void handle_dig_1_8(structures::client_s& client, structures::reader_s& in);
		void handle_place_1_8(structures::client_s& client, structures::reader_s& in);

	private:

		void change(structures::world_e target, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind);
		void send_change(structures::client_s& client, std::int32_t x, std::int32_t y, std::int32_t z, std::uint16_t kind);
		void restore_1_8(structures::client_s& client, std::int32_t x, std::int32_t y, std::int32_t z);

		static bool can_break(const structures::client_s& client, std::int32_t status);

		std::vector<std::uint8_t> packet;
	};

	extern java_block_c java_block;
}

//=====================================================================================
