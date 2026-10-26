
//=====================================================================================

#pragma once

#include "../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class bedrock_chunk_c
	{
	public:

		void build();
		void handle_subchunk_request(structures::session_s& session, structures::reader_s& in);
		const std::vector<std::uint8_t>& get_spawn_area() const;

	private:

		static void write_chunk(std::vector<std::uint8_t>& out, std::int32_t x, std::int32_t z);
		static void write_section(std::vector<std::uint8_t>& out);

		std::vector<std::uint8_t> spawn_area;
		std::vector<std::uint8_t> section;
		std::vector<std::uint8_t> packet;
		std::vector<std::uint8_t> body;
	};

	extern bedrock_chunk_c bedrock_chunk;
}

//=====================================================================================
