
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	void deflate_c::compress(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out, bool wrapped)
	{
		bit_buffer = 0ui64;
		bit_count = 0u;
		heads.fill(-1);

		if (wrapped)
		{
			out.push_back(0x78u);
			out.push_back(0x01u);
		}

		write_bits(out, 1u, 1u);
		write_bits(out, 1u, 2u);

		auto position{ std::size_t{ 0u } };

		while (position < size)
		{
			auto best_length{ 0u };
			auto best_distance{ 0u };

			if (position + deflate_min_match <= size)
			{
				auto slot{ hash(data + position) };
				auto candidate{ heads[slot] };
				auto limit{ static_cast<std::uint32_t>(std::min<std::size_t>(deflate_max_match, size - position)) };

				for (auto tries{ 0u }; tries < deflate_max_chain && candidate >= 0 && position - static_cast<std::size_t>(candidate) <= deflate_window && best_length < limit; ++tries)
				{
					auto length{ 0u };

					while (length < limit && data[static_cast<std::size_t>(candidate) + length] == data[position + length])
					{
						length += 1u;
					}

					if (length > best_length)
					{
						best_length = length;
						best_distance = static_cast<std::uint32_t>(position - static_cast<std::size_t>(candidate));
					}

					candidate = chain[static_cast<std::size_t>(candidate) & (deflate_window - 1u)];
				}

				chain[position & (deflate_window - 1u)] = heads[slot];
				heads[slot] = static_cast<std::int32_t>(position);
			}

			if (best_length >= deflate_min_match)
			{
				write_match(out, best_length, best_distance);

				for (auto step{ 1u }; step < best_length; ++step)
				{
					if (position + step + deflate_min_match <= size)
					{
						auto slot{ hash(data + position + step) };
						chain[(position + step) & (deflate_window - 1u)] = heads[slot];
						heads[slot] = static_cast<std::int32_t>(position + step);
					}
				}

				position += best_length;
			}

			else
			{
				write_literal(out, data[position]);
				position += 1u;
			}
		}

		write_literal(out, deflate_end_of_block);
		flush_bits(out);

		if (wrapped)
		{
			auto low{ 1u };
			auto high{ 0u };

			for (auto start{ std::size_t{ 0u } }; start < size; start += deflate_adler_block)
			{
				for (auto index{ start }; index < std::min(size, start + deflate_adler_block); ++index)
				{
					low += data[index];
					high += low;
				}

				low %= deflate_adler_modulus;
				high %= deflate_adler_modulus;
			}

			functions::write_u32(out, (high << 16u) | low);
		}
	}
	/*
	//=====================================================================================
	*/
	void deflate_c::write_bits(std::vector<std::uint8_t>& out, std::uint32_t value, std::uint32_t count)
	{
		bit_buffer |= static_cast<std::uint64_t>(value) << bit_count;
		bit_count += count;

		while (bit_count >= 8u)
		{
			out.push_back(static_cast<std::uint8_t>(bit_buffer));
			bit_buffer >>= 8u;
			bit_count -= 8u;
		}
	}
	/*
	//=====================================================================================
	*/
	void deflate_c::write_code(std::vector<std::uint8_t>& out, std::uint32_t code, std::uint32_t length)
	{
		auto reversed{ 0u };

		for (auto bit{ 0u }; bit < length; ++bit)
		{
			reversed |= ((code >> bit) & 1u) << (length - 1u - bit);
		}

		write_bits(out, reversed, length);
	}
	/*
	//=====================================================================================
	*/
	void deflate_c::write_literal(std::vector<std::uint8_t>& out, std::uint32_t literal)
	{
		if (literal < 144u)
		{
			write_code(out, 0x30u + literal, 8u);
		}

		else if (literal < 256u)
		{
			write_code(out, 0x190u + literal - 144u, 9u);
		}

		else if (literal < 280u)
		{
			write_code(out, literal - 256u, 7u);
		}

		else
		{
			write_code(out, 0xC0u + literal - 280u, 8u);
		}
	}
	/*
	//=====================================================================================
	*/
	void deflate_c::write_match(std::vector<std::uint8_t>& out, std::uint32_t length, std::uint32_t distance)
	{
		auto length_code{ 0u };
		auto distance_code{ 0u };

		while (length_code + 1u < std::size(deflate_length_base) && deflate_length_base[length_code + 1u] <= length)
		{
			length_code += 1u;
		}

		while (distance_code + 1u < std::size(deflate_distance_base) && deflate_distance_base[distance_code + 1u] <= distance)
		{
			distance_code += 1u;
		}

		write_literal(out, 257u + length_code);
		write_bits(out, length - deflate_length_base[length_code], deflate_length_extra[length_code]);
		write_code(out, distance_code, 5u);
		write_bits(out, distance - deflate_distance_base[distance_code], deflate_distance_extra[distance_code]);
	}
	/*
	//=====================================================================================
	*/
	void deflate_c::flush_bits(std::vector<std::uint8_t>& out)
	{
		if (bit_count)
		{
			out.push_back(static_cast<std::uint8_t>(bit_buffer));
		}

		bit_buffer = 0ui64;
		bit_count = 0u;
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t deflate_c::hash(const std::uint8_t* at)
	{
		return ((static_cast<std::uint32_t>(at[0]) << 16u | static_cast<std::uint32_t>(at[1]) << 8u | at[2]) * 2654435761u) >> (32u - deflate_hash_bits);
	}
}

//=====================================================================================
