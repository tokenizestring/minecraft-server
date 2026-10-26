
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_debug_c
	{
	public:

		void load();
		void inject(structures::session_s& session);

	private:

		std::vector<std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>>> inject_steps;
	};

	extern bedrock_debug_c bedrock_debug;
}

//=====================================================================================
