
//=====================================================================================

#pragma once

#include "../engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class deflate_c
	{
	public:

		void compress(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out, bool wrapped);

	private:

		void write_bits(std::vector<std::uint8_t>& out, std::uint32_t value, std::uint32_t count);
		void write_code(std::vector<std::uint8_t>& out, std::uint32_t code, std::uint32_t length);
		void write_literal(std::vector<std::uint8_t>& out, std::uint32_t literal);
		void write_match(std::vector<std::uint8_t>& out, std::uint32_t length, std::uint32_t distance);
		void flush_bits(std::vector<std::uint8_t>& out);

		static std::uint32_t hash(const std::uint8_t* at);

		std::array<std::int32_t, deflate_hash_size> heads;
		std::array<std::int32_t, deflate_window> chain;
		std::uint64_t bit_buffer;
		std::uint32_t bit_count;
	};
}

//=====================================================================================
