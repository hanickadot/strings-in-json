#ifndef NORMALIZE_HPP
#define NORMALIZE_HPP

#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <cstdint>
#include <array>

namespace json {

struct string_reader {
	char * current;
	const char * end;

	explicit constexpr string_reader(std::span<char> in) noexcept: current{in.data()}, end{in.data() + in.size()} { }
	explicit constexpr string_reader(std::string & in) noexcept: current{in.data()}, end{in.data() + in.size()} { }

	constexpr bool is_end() const noexcept {
		return current >= end;
	}

	constexpr char peek() const noexcept {
		assert(!is_end());
		return *current;
	}

	constexpr char peek(int off) const noexcept {
		assert(!is_end());
		return *(current + off);
	}

	constexpr void next() noexcept {
		++current;
	}

	constexpr void move(int v) noexcept {
		current += v;
	}

	constexpr size_t remaining() const noexcept {
		return static_cast<size_t>(std::distance(const_cast<const char *>(current), end));
	}

	constexpr auto writable_rest() const noexcept {
		return std::span<char>(current, remaining());
	}

	constexpr bool has_at_least(size_t n) const noexcept {
		return remaining() >= n;
	}

	constexpr bool read_character(char c) noexcept {
		if (is_end()) {
			return false;
		}

		if (*current != c) {
			return false;
		}

		++current;
		return true;
	}
};

constexpr uint8_t additional_length_of(char8_t first_unit) noexcept {
	return ((0x3A55000000000000ull >> ((unsigned(first_unit) >> 2u) & 0b111110u)) & 0b11u);
}

constexpr uint8_t additional_length_of_utf8_from_value(char32_t codepoint) noexcept {
	return (codepoint >= 0x10000u) + (codepoint >= 0x0800u) + (codepoint >= 0x80u);
}

consteval auto generate_escape_table() noexcept {
	std::array<char, 256> output;

	for (unsigned i = 0; i != output.size(); ++i) {
		char value = '\0';

		if (i == 'n') {
			value = '\n';
		} else if (i == 'r') {
			value = '\r';
		} else if (i == 't') {
			value = '\t';
		} else if (i == 'f') {
			value = '\f';
		} else if (i == 'b') {
			value = '\b';
		} else if (i == '/') {
			value = '/';
		} else if (i == '\\') {
			value = '\\';
		} else if (i == '"') {
			value = '"';
		}

		output[i] = value;
	}

	return output;
}

constexpr auto escape_table = generate_escape_table();

consteval auto generate_hexdec_table() noexcept {
	std::array<int8_t, 256> output;

	for (unsigned i = 0; i != output.size(); ++i) {
		int8_t value = -1;

		if (i >= 'a' && i <= 'f') {
			value = (i - 'a') + 10;
		} else if (i >= 'A' && i <= 'F') {
			value = (i - 'A') + 10;
		} else if (i >= '0' && i <= '9') {
			value = (i - '0');
		}

		output[i] = value;
	}

	return output;
}

constexpr auto hexdec_table = generate_hexdec_table();

constexpr int32_t convert_to_value(char a, char b, char c, char d) noexcept {
	const int32_t xa = hexdec_table[static_cast<uint8_t>(a)];
	const int32_t xb = hexdec_table[static_cast<uint8_t>(b)];
	const int32_t xc = hexdec_table[static_cast<uint8_t>(c)];
	const int32_t xd = hexdec_table[static_cast<uint8_t>(d)];
	return (xa << 12u) | (xb << 8u) | (xc << 4u) | xd;
}

constexpr bool is_valid_unicode_code_point(char32_t val) noexcept {
	return val <= 0x10FFFFul;
}

constexpr bool between(char32_t v, char32_t lo, char32_t hi) noexcept {
	return (v - lo) <= (hi - lo);
}

constexpr bool invalid_value(int32_t v) noexcept {
	return v < 0;
}

struct utf8_encode {
	std::array<uint8_t, 4> mask;
	std::array<uint8_t, 4> overlay;
};

constexpr auto utf8_encode_table = std::array<utf8_encode, 4>{
	utf8_encode{{0, 0, 0, 0b01111111u}, {0, 0, 0, 0b00000000u}},
	utf8_encode{{0, 0, 0b00011111u, 0b00111111u}, {0, 0, 0b11000000u, 0b10000000u}},
	utf8_encode{{0, 0b00001111u, 0b00111111u, 0b00111111u}, {0, 0b11100000u, 0b10000000u, 0b10000000u}},
	utf8_encode{{0b00000111u, 0b00111111u, 0b00111111u, 0b00111111u}, {0b11110000u, 0b10000000u, 0b10000000u, 0b10000000u}},
};

constexpr auto cut_to_lowest_8bits(char32_t in) noexcept {
	return static_cast<char>(static_cast<char8_t>(in));
}

constexpr void write_as_utf8_codepoint(char *& writer, char32_t cp) noexcept {
	assert(is_valid_unicode_code_point(cp));

	const uint8_t additional_bytes = additional_length_of_utf8_from_value(cp);

	assert(additional_bytes >= 0u);
	assert(additional_bytes <= 3u);
	const auto & utf8 = utf8_encode_table[additional_bytes];

	const uint64_t off_const = 0x0030201'00000000ull >> (additional_bytes * 8u);
	constexpr uint8_t default_overlay = 0b1000'0000ull;

	const uint8_t off0 = static_cast<uint8_t>(off_const);
	const uint8_t off1 = static_cast<uint8_t>(off_const >> 8);
	const uint8_t off2 = static_cast<uint8_t>(off_const >> 16);
	const uint8_t off3 = static_cast<uint8_t>(off_const >> 24);

	writer[off0] = cut_to_lowest_8bits(((cp >> 18u) & utf8.mask[0]) | utf8.overlay[0]);
	writer[off1] = cut_to_lowest_8bits(((cp >> 12u) & utf8.mask[1]) | utf8.overlay[1]);
	writer[off2] = cut_to_lowest_8bits(((cp >> 6u) & utf8.mask[2]) | utf8.overlay[2]);
	writer[off3] = cut_to_lowest_8bits(((cp >> 0u) & utf8.mask[3]) | utf8.overlay[3]);

	writer += (additional_bytes + 1u);
}

constexpr void write_as_utf8_codepoint_with_branch(char *& writer, char32_t cp) noexcept {
	assert(is_valid_unicode_code_point(cp));

	const uint8_t additional_bytes = additional_length_of_utf8_from_value(cp);

	assert(additional_bytes >= 0u);
	assert(additional_bytes <= 3u);

	if (additional_bytes == 0u) {
		writer[0] = cut_to_lowest_8bits(cp);
		writer += 1u;
	} else if (additional_bytes == 1u) {
		writer[0] = cut_to_lowest_8bits(0b110'00000u | (cp >> 6u));
		writer[1] = cut_to_lowest_8bits(0b10'000000u | (0b00'111111u & cp));
		writer += 2u;
	} else if (additional_bytes == 2u) {
		writer[0] = cut_to_lowest_8bits(0b1110'0000u | (cp >> 12u));
		writer[1] = cut_to_lowest_8bits(0b10'000000u | (0b00'111111u & (cp >> 6u)));
		writer[2] = cut_to_lowest_8bits(0b10'000000u | (0b00'111111u & cp));
		writer += 3u;
	} else {
		assert(additional_bytes == 3u);
		writer[0] = cut_to_lowest_8bits(0b11110'000u | (cp >> 18u));
		writer[1] = cut_to_lowest_8bits(0b10'000000u | (0b00'111111u & (cp >> 12u)));
		writer[2] = cut_to_lowest_8bits(0b10'000000u | (0b00'111111u & (cp >> 6u)));
		writer[3] = cut_to_lowest_8bits(0b10'000000u | (0b00'111111u & cp));
		writer += 4u;
	}
}

constexpr void copy_utf8_codepoint_to_output(char *& writer, string_reader & in, uint8_t number_of_additional_bytes) {
	assert(number_of_additional_bytes >= 0u);
	assert(number_of_additional_bytes <= 3u);

	const uint64_t off_const = 0x0030201'00000000ull >> (number_of_additional_bytes * 8u);

	// branchless copy of variable length utf8 code points...
	const uint8_t off0 = static_cast<uint8_t>(off_const);
	const uint8_t off1 = static_cast<uint8_t>(off_const >> 8);
	const uint8_t off2 = static_cast<uint8_t>(off_const >> 16);
	const uint8_t off3 = static_cast<uint8_t>(off_const >> 24);

	const char cu0 = in.peek(off0);
	const char cu1 = in.peek(off1);
	const char cu2 = in.peek(off2);
	const char cu3 = in.peek(off3);

	writer[off0] = cu0;
	writer[off1] = cu1;
	writer[off2] = cu2;
	writer[off3] = cu3;

	writer += number_of_additional_bytes + 1u;
	in.move(number_of_additional_bytes + 1u);
}

constexpr bool copy_utf8_codepoint_to_output_branch(char *& writer, string_reader & in, uint8_t number_of_additional_bytes) noexcept {
	assert(number_of_additional_bytes >= 0u);
	assert(number_of_additional_bytes <= 3u);

	// check if there was an error, this type of code unit is not allowed and must be second or farther...
	bool error = (static_cast<char8_t>(*in.current) & 0b11'000000u) == 0b10'000000;

	// copy each byte
	*writer++ = *in.current++;

	if (number_of_additional_bytes == 0) [[likely]] {
		return error;
	}

	error |= (static_cast<char8_t>(*in.current) & 0b11'000000u) != 0b10'000000;
	*writer++ = *in.current++;

	if (number_of_additional_bytes == 1) [[likely]] {
		return error;
	}

	error |= (static_cast<char8_t>(*in.current) & 0b11'000000u) != 0b10'000000;
	*writer++ = *in.current++;

	if (number_of_additional_bytes == 2) [[likely]] {
		return error;
	}

	error |= (static_cast<char8_t>(*in.current) & 0b11'000000u) != 0b10'000000;
	*writer++ = *in.current++;

	return error;
}

template <bool Branchless = false> [[gnu::flatten]] constexpr auto read_and_normalize_string(string_reader & in, std::span<char> output) -> std::optional<std::string_view> {
	if (!in.read_character('"')) {
		return std::nullopt;
	}

	char * writer = output.data();

	// we loop thru
	for (;;) {
		if (in.is_end()) [[unlikely]] {
			throw std::invalid_argument("unexpected end");
			return std::nullopt;
		}

		const char c = in.peek();

		// end of string
		if (in.peek() == '"') [[unlikely]] {
			break;
		}

		// json string escape
		if (in.peek() == '\\') [[unlikely]] {
			in.next();
			if (in.is_end()) [[unlikely]] {
				throw std::invalid_argument("unexpected end after escape");
				return std::nullopt;
			}

			const char c2 = in.peek();
			in.next();

			// use replacament table for simple one-char escapes
			if (const char replacement = escape_table[static_cast<char8_t>(c2)]) [[likely]] {
				assert(writer < (output.data() + output.size()));
				*writer++ = replacement;
				continue;
			}

			// there can be only unicode escape now and it must be at least 4 characters + 1 for end quote
			if ((c2 != 'u') || !in.has_at_least(5u)) [[unlikely]] {
				throw std::invalid_argument("not enough space");
				return std::nullopt;
			}

			// read 4 hexdec characters and convert into a number
			const auto h = convert_to_value(in.peek(0), in.peek(1), in.peek(2), in.peek(3));
			in.move(4);

			if (invalid_value(h)) [[unlikely]] {
				// invalid hexdec value
				throw std::invalid_argument("invalid hexdec escape");
				return std::nullopt;
			}

			char32_t cp = static_cast<uint16_t>(h);

			// if it's a surrogate pair, it's a special case, there is no other way how to encode values over 0xFFFFu
			if (between(cp, 0xD800u, 0xDBFFu)) [[unlikely]] {
				// and if there is not at least 6+1 characters (surrogate pair + end, we can fail)
				if (!in.has_at_least(7u) || ((in.peek() != '\\') | (in.peek(1) != 'u'))) [[unlikely]] {
					// something else than pair
					throw std::invalid_argument("not following lo-surrogate");
					return std::nullopt;
				}

				// read 4 hexdec characters and convert into a number
				const auto l = convert_to_value(in.peek(2), in.peek(3), in.peek(4), in.peek(5));
				in.move(6);

				const char32_t lo = static_cast<uint16_t>(l);

				if (invalid_value(l)) [[unlikely]] {
					// invalid hexdec value
					throw std::invalid_argument("invalid hexdec in lo-surrogate");
					return std::nullopt;
				}

				if (!between(lo, 0xDC00u, 0xDFFFu)) [[unlikely]] {
					throw std::invalid_argument("not lo-surrogate value");
					return std::nullopt;
				}

				constexpr char32_t lower_10_bits = 0b11111'11111ul;
				cp = ((cp & lower_10_bits) << 10u) + (lo & lower_10_bits) + 0x10000ul;

				// resulting character must be valid utf-32 code point
				if (!is_valid_unicode_code_point(cp)) [[unlikely]] {
					throw std::invalid_argument("invalid codepoint");
					return std::nullopt;
				}
			}

			// it's given
			if constexpr (Branchless) {
				write_as_utf8_codepoint(writer, cp);
			} else {
				write_as_utf8_codepoint_with_branch(writer, cp);
			}

			continue;
		}

		// handle normal utf-8 unicode (copy each code-point and validate)

		const uint8_t number_of_additional_bytes = additional_length_of(static_cast<char8_t>(c));

		// number of bytes of code_point + current + end quote, otherwise we can end
		if (!in.has_at_least(number_of_additional_bytes + 2u)) [[unlikely]] {
			throw std::invalid_argument("not enough space");
			return std::nullopt;
		}

		if constexpr (Branchless) {
			copy_utf8_codepoint_to_output(writer, in, number_of_additional_bytes);
		} else {
			if (copy_utf8_codepoint_to_output_branch(writer, in, number_of_additional_bytes)) {
				throw std::invalid_argument("invalid utf8");
				return std::nullopt;
			}
		}
	}

	// return writed part as it's now normalized
	return std::string_view(output.data(), static_cast<size_t>(std::distance(output.data(), writer)));
}

template <bool Branchless = false> [[gnu::flatten]] constexpr auto read_and_normalize_string(string_reader & in) -> std::optional<std::string_view> {
	return read_and_normalize_string<Branchless>(in, in.writable_rest());
}

} // namespace json

#endif
