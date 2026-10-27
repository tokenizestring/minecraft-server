
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	blocks_c blocks;

	void blocks_c::load()
	{
		auto count{ 0u };

		if (auto file{ std::fopen(functions::get_local_path(gen_block_path).c_str(), "rb") }; file)
		{
			std::array<char, 512u> line{};
			std::array<char, 64u> key{};
			std::array<char, 256u> state{};

			while (std::fgets(line.data(), static_cast<std::int32_t>(line.size()), file))
			{
				std::uint32_t id_26_2{};
				std::uint32_t id_26_3{};
				std::uint32_t legacy{};
				std::uint32_t filter{};
				std::uint32_t fluid{};

				if (count < structures::block_count && std::sscanf(line.data(), "%63s %255s %u %u %u %u %u", key.data(), state.data(), &id_26_2, &id_26_3, &legacy, &filter, &fluid) == 7)
				{
					ids[structures::family_26_2][count] = id_26_2;
					ids[structures::family_26_3][count] = id_26_3;
					ids[structures::family_1_8][count] = legacy;
					filters[count] = static_cast<std::uint8_t>(filter);
					fluids[count] = fluid != 0u;
					count += 1u;
				}
			}

			std::fclose(file);
		}

		if (count == structures::block_count)
		{
			utils::log.write(color_server, "[blocks](load) %u generated block states for 26.2, 26.3 and 1.8", count);
		}

		else
		{
			utils::log.write(color_error, "[blocks](load) '%s' has %u of %u blocks, generated worlds will be wrong", gen_block_path, count, static_cast<std::uint32_t>(structures::block_count));
		}
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t blocks_c::get_java(structures::family_e family, std::uint8_t block) const
	{
		return ids[family][block];
	}
	/*
	//=====================================================================================
	*/
	std::uint8_t blocks_c::get_filter(std::uint8_t block) const
	{
		return filters[block];
	}
	/*
	//=====================================================================================
	*/
	bool blocks_c::is_fluid(std::uint8_t block) const
	{
		return fluids[block];
	}
}

//=====================================================================================
