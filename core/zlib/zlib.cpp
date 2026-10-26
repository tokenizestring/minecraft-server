
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	zlib_c zlib;

	bool zlib_c::inflate_raw(const std::uint8_t* data, std::size_t size, std::vector<std::uint8_t>& out, std::size_t limit)
	{
		input = data;
		input_size = size;
		position = 0u;
		bit_buffer = 0u;
		bit_count = 0u;
		fault = false;
		out.clear();

		auto last{ false };

		while (!last && !fault && out.size() <= limit)
		{
			last = (read_bits(1u) == 1u);

			switch (read_bits(2u))
			{
			case 0u:
				inflate_stored(out, limit);
				break;

			case 1u:
				inflate_fixed(out, limit);
				break;

			case 2u:
				inflate_dynamic(out, limit);
				break;

			case 3u:
				fault = true;
				break;
			}
		}

		return (!fault && out.size() <= limit);
	}
	/*
	//=====================================================================================
	*/
	std::uint32_t zlib_c::read_bits(std::uint32_t count)
	{
		while (bit_count < count && position < input_size)
		{
			bit_buffer |= static_cast<std::uint32_t>(input[position++]) << bit_count;
			bit_count += 8u;
		}

		auto value{ 0u };

		if (bit_count >= count)
		{
			value = bit_buffer & ((1u << count) - 1u);
			bit_buffer = count < 32u ? bit_buffer >> count : 0u;
			bit_count -= count;
		}

		else
		{
			fault = true;
		}

		return value;
	}
	/*
	//=====================================================================================
	*/
	std::int32_t zlib_c::decode(const structures::huffman_s& table)
	{
		auto code{ 0 };
		auto first{ 0 };
		auto index{ 0 };
		auto symbol{ -1 };

		for (auto length{ 1u }; length <= deflate_max_bits && symbol < 0 && !fault; ++length)
		{
			code |= static_cast<std::int32_t>(read_bits(1u));

			if (auto count{ static_cast<std::int32_t>(table.counts[length]) }; code - count < first)
			{
				symbol = table.symbols[static_cast<std::size_t>(index + (code - first))];
			}

			else
			{
				index += count;
				first = (first + count) << 1;
				code <<= 1;
			}
		}

		if (symbol < 0)
		{
			fault = true;
		}

		return symbol;
	}
	/*
	//=====================================================================================
	*/
	bool zlib_c::build(structures::huffman_s& table, const std::int16_t* lengths, std::uint32_t count)
	{
		table.counts.fill(0);

		for (auto index{ 0u }; index < count; ++index)
		{
			++table.counts[static_cast<std::size_t>(lengths[index])];
		}

		std::array<std::int16_t, deflate_max_bits + 1u> offsets{};
		auto left{ 1 };

		for (auto length{ 1u }; length <= deflate_max_bits; ++length)
		{
			left = (left << 1) - table.counts[length];
		}

		for (auto length{ 1u }; length < deflate_max_bits; ++length)
		{
			offsets[length + 1u] = static_cast<std::int16_t>(offsets[length] + table.counts[length]);
		}

		for (auto index{ 0u }; index < count; ++index)
		{
			if (lengths[index])
			{
				table.symbols[static_cast<std::size_t>(offsets[static_cast<std::size_t>(lengths[index])]++)] = static_cast<std::int16_t>(index);
			}
		}

		return (left >= 0);
	}
	/*
	//=====================================================================================
	*/
	void zlib_c::inflate_stored(std::vector<std::uint8_t>& out, std::size_t limit)
	{
		bit_buffer = 0u;
		bit_count = 0u;

		if (position + 4u <= input_size)
		{
			auto length{ static_cast<std::uint32_t>(input[position] | (input[position + 1u] << 8u)) };
			auto inverse{ static_cast<std::uint32_t>(input[position + 2u] | (input[position + 3u] << 8u)) };
			position += 4u;

			if (length == (~inverse & 0xFFFFu) && position + length <= input_size && out.size() + length <= limit)
			{
				out.insert(out.end(), input + position, input + position + length);
				position += length;
			}

			else
			{
				fault = true;
			}
		}

		else
		{
			fault = true;
		}
	}
	/*
	//=====================================================================================
	*/
	void zlib_c::inflate_codes(std::vector<std::uint8_t>& out, std::size_t limit, const structures::huffman_s& lengths, const structures::huffman_s& distances)
	{
		auto symbol{ 0 };

		while (symbol != 256 && !fault && out.size() <= limit)
		{
			symbol = decode(lengths);

			if (symbol >= 0 && symbol < 256)
			{
				out.push_back(static_cast<std::uint8_t>(symbol));
			}

			else if (symbol > 256 && symbol - 257 < static_cast<std::int32_t>(std::size(deflate_length_base)))
			{
				auto length{ deflate_length_base[symbol - 257] + read_bits(deflate_length_extra[symbol - 257]) };

				if (auto code{ decode(distances) }; code >= 0 && code < static_cast<std::int32_t>(std::size(deflate_distance_base)))
				{
					auto distance{ deflate_distance_base[code] + read_bits(deflate_distance_extra[code]) };

					if (distance <= out.size() && out.size() + length <= limit + 1u)
					{
						for (auto index{ 0u }; index < length; ++index)
						{
							out.push_back(out[out.size() - distance]);
						}
					}

					else
					{
						fault = true;
					}
				}

				else
				{
					fault = true;
				}
			}

			else if (symbol != 256)
			{
				fault = true;
			}
		}
	}
	/*
	//=====================================================================================
	*/
	void zlib_c::inflate_fixed(std::vector<std::uint8_t>& out, std::size_t limit)
	{
		if (fixed_ready)
		{
			inflate_codes(out, limit, fixed_lengths, fixed_distances);
		}

		else
		{
			std::array<std::int16_t, 288u> lengths{};

			for (auto index{ 0u }; index < lengths.size(); ++index)
			{
				lengths[index] = static_cast<std::int16_t>(index < 144u ? 8 : index < 256u ? 9 : index < 280u ? 7 : 8);
			}

			build(fixed_lengths, lengths.data(), 288u);
			lengths.fill(5);
			build(fixed_distances, lengths.data(), 30u);
			fixed_ready = true;

			inflate_codes(out, limit, fixed_lengths, fixed_distances);
		}
	}
	/*
	//=====================================================================================
	*/
	void zlib_c::inflate_dynamic(std::vector<std::uint8_t>& out, std::size_t limit)
	{
		auto length_count{ read_bits(5u) + 257u };
		auto distance_count{ read_bits(5u) + 1u };
		auto code_count{ read_bits(4u) + 4u };

		std::array<std::int16_t, 320u> lengths{};

		for (auto index{ 0u }; index < code_count; ++index)
		{
			lengths[deflate_code_order[index]] = static_cast<std::int16_t>(read_bits(3u));
		}

		if (length_count <= 286u && distance_count <= 30u && build(dynamic_lengths, lengths.data(), 19u))
		{
			auto index{ 0u };

			while (index < length_count + distance_count && !fault)
			{
				auto symbol{ decode(dynamic_lengths) };
				auto repeat{ 0u };
				std::int16_t value{ 0 };

				if (symbol >= 0 && symbol < 16)
				{
					lengths[index++] = static_cast<std::int16_t>(symbol);
				}

				else if (symbol == 16 && index)
				{
					value = lengths[index - 1u];
					repeat = 3u + read_bits(2u);
				}

				else if (symbol == 17)
				{
					repeat = 3u + read_bits(3u);
				}

				else if (symbol == 18)
				{
					repeat = 11u + read_bits(7u);
				}

				else
				{
					fault = true;
				}

				if (index + repeat <= length_count + distance_count)
				{
					for (; repeat; --repeat)
					{
						lengths[index++] = value;
					}
				}

				else
				{
					fault = true;
				}
			}

			if (!fault && lengths[256] && build(dynamic_lengths, lengths.data(), length_count))
			{
				build(dynamic_distances, lengths.data() + length_count, distance_count);
				inflate_codes(out, limit, dynamic_lengths, dynamic_distances);
			}

			else
			{
				fault = true;
			}
		}

		else
		{
			fault = true;
		}
	}
}

//=====================================================================================
