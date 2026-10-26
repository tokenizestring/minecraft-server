
//=====================================================================================

#pragma once

#include "../engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class zlib_c
	{
	public:

		bool inflate_raw(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out, std::size_t limit);

	private:

		std::uint32_t read_bits(std::uint32_t count);
		std::int32_t decode(const structures::huffman_s& table);
		bool build(structures::huffman_s& table, const std::int16_t* lengths, std::uint32_t count);
		void inflate_stored(std::vector<std::uint8_t>& out, std::size_t limit);
		void inflate_codes(std::vector<std::uint8_t>& out, std::size_t limit, const structures::huffman_s& lengths, const structures::huffman_s& distances);
		void inflate_fixed(std::vector<std::uint8_t>& out, std::size_t limit);
		void inflate_dynamic(std::vector<std::uint8_t>& out, std::size_t limit);

		const std::uint8_t* input;
		std::size_t input_size;
		std::size_t position;
		std::uint32_t bit_buffer;
		std::uint32_t bit_count;
		bool fault;

		structures::huffman_s fixed_lengths;
		structures::huffman_s fixed_distances;
		structures::huffman_s dynamic_lengths;
		structures::huffman_s dynamic_distances;
		bool fixed_ready;
	};

	extern zlib_c zlib;
}

//=====================================================================================
