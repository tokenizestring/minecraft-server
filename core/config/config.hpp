
//=====================================================================================

#pragma once

#include "../engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class config_c
	{
	public:

		void load();
		void store() const;
		bool is_operator(std::string_view name) const;

		std::uint16_t java_port{ 25565u };
		std::uint16_t bedrock_port{ 19132u };
		std::uint32_t max_players{ 100u };
		std::string motd{ "minecraft server" };
		std::uint32_t chat_lines{ 10u };
		std::uint64_t chat_fade_ms{ 15000ui64 };
		std::uint32_t view_distance{ 6u };
		std::uint32_t chunk_cache_mb{ 32u };
		std::uint32_t chunk_threads{ 0u };
		std::uint32_t chunk_budget{ 8u };
		std::string operators;
		std::uint64_t smp_seed{ 0ui64 };
		std::uint64_t start_balance{ 100ui64 };
		std::uint64_t max_price{ 1000000000ui64 };

	private:

		void apply(std::string_view key, std::string_view value);
		void save(const std::string& path) const;

		static std::string_view trim(std::string_view text);
		static std::uint64_t read_number(std::string_view value, std::uint64_t fallback, std::uint64_t low, std::uint64_t high);
	};

	extern config_c config;
}

//=====================================================================================
