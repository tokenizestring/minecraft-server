
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
		void handle_dig_1_8(structures::client_s& client, structures::reader_s& in);
		void handle_place_1_8(structures::client_s& client, structures::reader_s& in);

	private:

		void restore_1_8(structures::client_s& client, std::int32_t x, std::int32_t y, std::int32_t z);

		std::vector<std::uint8_t> packet;
	};

	extern java_block_c java_block;
}

//=====================================================================================
