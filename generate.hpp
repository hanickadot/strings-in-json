#ifndef GENERATE_HPP
#define GENERATE_HPP

#include "normalize.hpp"
#include <random>
#include <string>
#include <cstddef>
#include <iostream>

std::string generate_random_json_string_with_length(size_t bytes) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::discrete_distribution<uint8_t> random_byte_length({200, 20, 1, 1, 2, 2});
	std::array<std::uniform_int_distribution<uint32_t>, 4> random_character{
		std::uniform_int_distribution<uint32_t>(0x20u, 0x7Fu),
		std::uniform_int_distribution<uint32_t>(0x80u, 0x7FFu),
		std::uniform_int_distribution<uint32_t>(0x800u, 0xFFFFu),
		std::uniform_int_distribution<uint32_t>(0x10000u, 0x1FFFFu),
	};

	const auto escapes = std::array<char, 8>{'n', 'r', 't', 'f', 'b', '/', '\\', '"'};
	std::uniform_int_distribution<uint32_t> random_escape{0, escapes.size() - 1u};
	std::uniform_int_distribution<uint16_t> random_ucs2{0x20u, 0xFFFFu};
	std::uniform_int_distribution<uint32_t> random_lo{0xDC00u, 0xDFFFu};

	std::geometric_distribution<uint32_t> max_four_bytes_geo{};

	constexpr char hexdec[] = "0123456789abcdef";

	std::string output;
	output.resize(bytes);

	char * it = output.data();
	const char * end = output.data() + output.size();

	*it++ = '"';

	for (;;) {
		const auto remaining = std::distance(const_cast<const char *>(it), end);

		if (remaining == 1) {
			break;
		}

		const uint8_t bytes = random_byte_length(gen);

		if (bytes == 4u) {
			if (2 >= (static_cast<size_t>(remaining) - 1u)) {
				continue;
			}
			*it++ = '\\';
			*it++ = escapes[random_escape(gen)];
			continue;
		} else if (bytes == 5u) {
			if (6 >= (static_cast<size_t>(remaining) - 1u)) {
				continue;
			}
			*it++ = '\\';
			*it++ = 'u';
			for (;;) {
				const uint16_t value = random_ucs2(gen);

				if (value >= 0xDC00u && value <= 0xDFFFu) {
					continue;
				}

				if (value >= 0xD800u && value <= 0xDBFFu) {
					// high surrogate implies more space is needed
					if ((12u) >= (static_cast<size_t>(remaining) - 1u)) {
						continue;
					}
				}

				*it++ = hexdec[(value >> 12u) & 0xFu];
				*it++ = hexdec[(value >> 8u) & 0xFu];
				*it++ = hexdec[(value >> 4u) & 0xFu];
				*it++ = hexdec[value & 0xFu];

				// it was high surrogate
				if (value >= 0xD800u && value <= 0xDBFFu) {
					const uint16_t lo = random_lo(gen);
					*it++ = '\\';
					*it++ = 'u';
					*it++ = hexdec[(lo >> 12u) & 0xFu];
					*it++ = hexdec[(lo >> 8u) & 0xFu];
					*it++ = hexdec[(lo >> 4u) & 0xFu];
					*it++ = hexdec[lo & 0xFu];
				}
				break;
			}

			continue;
		}

		if (bytes >= (static_cast<size_t>(remaining) - 1u)) {
			continue;
		}

		const char32_t value = static_cast<char32_t>(random_character[bytes](gen));

		if (value == '\\') {
			continue;
		}

		if (value == '"') {
			continue;
		}

		if ((value >= 0xD800u) && (value <= 0xDFFFu)) {
			// skip surrogates
			continue;
		}

		json::write_as_utf8_codepoint(it, value);
	}

	*it++ = '"';

	assert(it == end);

	return output;
}

#endif