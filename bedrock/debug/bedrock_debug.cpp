
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	bedrock_debug_c bedrock_debug;

	void bedrock_debug_c::inject(structures::session_s& session)
	{
		if (bedrock_debug_inject && session.playing && session.inject_step < inject_steps.size() && functions::get_time_ms() >= session.inject_ms)
		{
			for (const auto& [id, bytes] : inject_steps[session.inject_step])
			{
				bedrock_packet.send(session, id, bytes);
			}

			utils::log.write(color_warn, "[debug](inject) '%s' step %u of %zu, %zu packets", session.name.c_str(), session.inject_step, inject_steps.size(), inject_steps[session.inject_step].size());
			session.inject_step += 1u;
			session.inject_ms = functions::get_time_ms() + bedrock_inject_interval_ms;
		}
	}
	/*
	//=====================================================================================
	*/
	void bedrock_debug_c::load()
	{
		if (auto list{ std::fopen(functions::get_local_path("../../tools/captures/inject/list.txt").c_str(), "rb") }; list && bedrock_debug_inject)
		{
			std::array<char, 512u> line{};
			std::uint32_t step{};
			std::uint32_t id{};
			std::array<char, 256u> name{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), list))
			{
				if (std::sscanf(line.data(), "%u %u %255s", &step, &id, name.data()) == 3)
				{
					if (auto file{ std::fopen(functions::get_local_path((std::string{ "../../tools/captures/inject/" } + name.data()).c_str()).c_str(), "rb") }; file)
					{
						std::vector<std::uint8_t> bytes(1048576u);
						bytes.resize(std::fread(bytes.data(), 1u, bytes.size(), file));
						std::fclose(file);
						inject_steps.resize(std::max<std::size_t>(inject_steps.size(), step + 1u));
						inject_steps[step].emplace_back(id, std::move(bytes));
					}
				}
			}

			std::fclose(list);
			utils::log.write(color_warn, "[debug](inject) loaded %zu steps", inject_steps.size());
		}
	}
}

//=====================================================================================
