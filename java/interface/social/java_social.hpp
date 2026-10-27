
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_social_c
	{
	public:

		void open(structures::client_s& client);
		void click(structures::client_s& client, std::int32_t slot, std::int32_t button, std::int32_t mode);

	private:

		void render(structures::client_s& client);

		std::vector<std::uint8_t> packet;
	};

	extern java_social_c java_social;
}

//=====================================================================================
