
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_market_c
	{
	public:

		void open_buy(structures::session_s& session, std::string_view filter);
		void open_sell(structures::session_s& session);
		void choose(structures::session_s& session, std::uint32_t button);
		void sell(structures::session_s& session, std::string_view data);

	private:

		void send(structures::session_s& session, std::uint32_t id);

		std::string form;
		std::vector<std::uint8_t> body;
	};

	extern bedrock_market_c bedrock_market;
}

//=====================================================================================
