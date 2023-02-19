#include "normalize.hpp"
#include <sstream>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iostream>

std::string repeat(std::string_view input, int n) {
	std::ostringstream os;
	os << "\"";
	for (int i = 0; i != n; ++i) {
		os << input;
	}
	os << "\"";
	return std::move(os).str();
}

std::optional<std::string_view> normalize(std::string & content) {
	auto reader = json::string_reader(content);
	const auto view = json::read_and_normalize_string(reader);
	return view;
}

using namespace std::string_view_literals;

TEST_CASE("unicode length") {
	for (unsigned i = 0; i != 128; ++i) {
		REQUIRE(json::additional_length_of(char8_t(i)) == 0u);
	}

	for (unsigned i = 0; i != 32; ++i) {
		REQUIRE(json::additional_length_of(char8_t(i) | 0b110'00000u) == 1u);
	}

	for (unsigned i = 0; i != 16; ++i) {
		REQUIRE(json::additional_length_of(char8_t(i) | 0b1110'0000u) == 2u);
	}

	for (unsigned i = 0; i != 8; ++i) {
		REQUIRE(json::additional_length_of(char8_t(i) | 0b11110'000u) == 3u);
	}
}

TEST_CASE("basics") {
	constexpr auto in = R"(hello there \n\r\t \uD83D\uDE00 ěščřž uff 😀\u2192\u2211\u0394aabbccdde\\ĚŠČŘŽÝ😀😀)"sv;
	REQUIRE(in.size() == 100u);

	std::string str = "\"" + std::string{in} + "\"";
	const auto val = normalize(str);
	REQUIRE(val.has_value());
	REQUIRE((*val == "hello there \n\r\t 😀 ěščřž uff 😀→∑Δaabbccdde\\ĚŠČŘŽÝ😀😀"sv));

	BENCHMARK_ADVANCED("100B")
	(Catch::Benchmark::Chronometer meter) {
		auto content = repeat(in, 1);

		meter.measure([&] {
			const auto out = normalize(content);
			return out;
		});
	};

	BENCHMARK_ADVANCED("100kB")
	(Catch::Benchmark::Chronometer meter) {
		auto content = repeat(in, 1024);

		meter.measure([&] {
			const auto out = normalize(content);
			return out;
		});
	};

	BENCHMARK_ADVANCED("100MB")
	(Catch::Benchmark::Chronometer meter) {
		auto content = repeat(in, 1024 * 1024);

		meter.measure([&] {
			const auto out = normalize(content);
			return out;
		});
	};
}