
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	config_c config;

	void config_c::load()
	{
		auto path{ functions::get_local_path(config_file_name) };

		if (auto file{ std::fopen(path.c_str(), "rb") }; file)
		{
			std::array<char, 1024u> line{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				auto text{ trim(line.data()) };

				if (auto equals{ text.find('=') }; equals != std::string_view::npos && text.size() && text[0] != '#')
				{
					apply(trim(text.substr(0u, equals)), trim(text.substr(equals + 1u)));
				}
			}

			std::fclose(file);
			utils::log.write(color_server, "[config](load) '%s'", config_file_name);
		}

		else
		{
			utils::log.write(color_server, "[config](create) '%s' with defaults", config_file_name);
		}

		smp_seed = smp_seed ? smp_seed : functions::make_seed();
		save(path);

		utils::log.write(color_server, "[config] java port %u, bedrock port %u, max players %u, view distance %u, chunk cache %u mb, chat lines %u, chat fade %llu s", java_port, bedrock_port, max_players, view_distance, chunk_cache_mb, chat_lines, chat_fade_ms / 1000ui64);
	}
	/*
	//=====================================================================================
	*/
	void config_c::apply(std::string_view key, std::string_view value)
	{
		if (key == "java-port")
		{
			java_port = static_cast<std::uint16_t>(read_number(value, java_port, 1u, 65535u));
		}

		else if (key == "bedrock-port")
		{
			bedrock_port = static_cast<std::uint16_t>(read_number(value, bedrock_port, 1u, 65535u));
		}

		else if (key == "max-players")
		{
			max_players = static_cast<std::uint32_t>(read_number(value, max_players, 1u, 100000u));
		}

		else if (key == "motd")
		{
			motd.assign(value.substr(0u, max_motd_length));
			std::erase_if(motd, [](auto character) { return character == '"' || character == '\\' || static_cast<std::uint8_t>(character) < 0x20u; });
		}

		else if (key == "view-distance")
		{
			view_distance = static_cast<std::uint32_t>(read_number(value, view_distance, 2u, 32u));
		}

		else if (key == "chunk-cache-mb")
		{
			chunk_cache_mb = static_cast<std::uint32_t>(read_number(value, chunk_cache_mb, 1u, 4096u));
		}

		else if (key == "chunk-threads")
		{
			chunk_threads = static_cast<std::uint32_t>(read_number(value, chunk_threads, 0u, 64u));
		}

		else if (key == "chunk-budget")
		{
			chunk_budget = static_cast<std::uint32_t>(read_number(value, chunk_budget, 1u, 256u));
		}

		else if (key == "operators")
		{
			operators.assign(value);
		}

		else if (key == "smp-seed")
		{
			smp_seed = read_number(value, smp_seed, 1u, 0x7FFFFFFFFFFFFFFFui64);
		}

		else if (key == "start-balance")
		{
			start_balance = read_number(value, start_balance, 0u, 1000000000ui64);
		}

		else if (key == "max-price")
		{
			max_price = read_number(value, max_price, 1u, 1000000000000ui64);
		}

		else if (key == "chat-lines")
		{
			chat_lines = static_cast<std::uint32_t>(read_number(value, chat_lines, 1u, max_chat_lines));
		}

		else if (key == "chat-fade-seconds")
		{
			chat_fade_ms = read_number(value, chat_fade_ms / 1000ui64, 1u, 3600u) * 1000ui64;
		}

		else
		{
			utils::log.write(color_warn, "[config] unknown setting '%.*s'", static_cast<std::int32_t>(key.size()), key.data());
		}
	}
	/*
	//=====================================================================================
	*/
	void config_c::save(const std::string& path) const
	{
		if (auto file{ std::fopen(path.c_str(), "wb") }; file)
		{
			std::fprintf(file, "# server settings, restart the server after editing\n\n");
			std::fprintf(file, "java-port=%u\n", java_port);
			std::fprintf(file, "bedrock-port=%u\n", bedrock_port);
			std::fprintf(file, "max-players=%u\n", max_players);
			std::fprintf(file, "motd=%s\n\n", motd.c_str());
			std::fprintf(file, "# chunks sent around each java player, and memory kept for ready-made chunks\n");
			std::fprintf(file, "view-distance=%u\n", view_distance);
			std::fprintf(file, "chunk-cache-mb=%u\n", chunk_cache_mb);
			std::fprintf(file, "# threads that build chunks (0 = all cores but one), chunks sent per player per tick\n");
			std::fprintf(file, "chunk-threads=%u\n", chunk_threads);
			std::fprintf(file, "chunk-budget=%u\n\n", chunk_budget);
			std::fprintf(file, "# names allowed to use /gamemode, separated by commas\n");
			std::fprintf(file, "operators=%s\n\n", operators.c_str());
			std::fprintf(file, "# seed of the smp world, /roll picks a new one\n");
			std::fprintf(file, "smp-seed=%llu\n\n", smp_seed);
			std::fprintf(file, "# money new players start with, and the highest price allowed on /sell\n");
			std::fprintf(file, "start-balance=%llu\n", start_balance);
			std::fprintf(file, "max-price=%llu\n\n", max_price);
			std::fprintf(file, "# chat sidebar: lines shown (1-15) and seconds before a line fades\n");
			std::fprintf(file, "chat-lines=%u\n", chat_lines);
			std::fprintf(file, "chat-fade-seconds=%llu\n", chat_fade_ms / 1000ui64);
			std::fclose(file);
		}
	}
	/*
	//=====================================================================================
	*/
	void config_c::store() const
	{
		save(functions::get_local_path(config_file_name));
	}
	/*
	//=====================================================================================
	*/
	std::string_view config_c::trim(std::string_view text)
	{
		auto start{ text.find_first_not_of(" \t\r\n") };
		auto end{ text.find_last_not_of(" \t\r\n") };

		return (start == std::string_view::npos) ? std::string_view{} : text.substr(start, end - start + 1u);
	}
	/*
	//=====================================================================================
	*/
	std::uint64_t config_c::read_number(std::string_view value, std::uint64_t fallback, std::uint64_t low, std::uint64_t high)
	{
		auto number{ fallback };
		auto result{ fallback };

		if (auto [end, error]{ std::from_chars(value.data(), value.data() + value.size(), number) }; error == std::errc{} && number >= low && number <= high)
		{
			result = number;
		}

		else
		{
			utils::log.write(color_warn, "[config] bad value '%.*s', keeping %llu", static_cast<std::int32_t>(value.size()), value.data(), fallback);
		}

		return result;
	}
	/*
	//=====================================================================================
	*/
	bool config_c::is_operator(std::string_view name) const
	{
		auto found{ false };
		auto start{ 0u };

		while (start <= operators.size() && name.size())
		{
			auto end{ std::min(operators.find(',', start), operators.size()) };
			found = found || trim(std::string_view{ operators }.substr(start, end - start)) == name;
			start = static_cast<std::uint32_t>(end) + 1u;
		}

		return found;
	}
}

//=====================================================================================
