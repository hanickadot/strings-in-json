#include "generate.hpp"
#include "normalize.hpp"
#include <iterator>
#include <random>
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

TEST_CASE("hexdec") {
	constexpr auto alphabet = std::array<char, 22>{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'A', 'B', 'C', 'D', 'E', 'F'};

	for (char a: alphabet) {
		for (char b: alphabet) {
			for (char c: alphabet) {
				for (char d: alphabet) {
					REQUIRE(json::convert_to_value(a, b, c, d) >= 0);
				}
			}
		}
	}
}

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

TEST_CASE("basics (branchless)") {
	auto normalize = [](std::string & content) {
		auto reader = json::string_reader(content);
		const auto view = json::read_and_normalize_string<true>(reader);
		return view;
	};

	auto r = generate_random_json_string_with_length(100);
	REQUIRE(r.size() == 100u);
	const auto val0 = normalize(r);
	REQUIRE(val0.has_value());

	constexpr auto in = R"(hello there \n\r\t \uD83D\uDE00 ěščřž uff 😀\u2192\u2211\u0394aabbccdde\\ĚŠČŘŽÝ😀😀)"sv;
	REQUIRE(in.size() == 100u);

	std::string str = "\"" + std::string{in} + "\"";
	const auto val = normalize(str);
	REQUIRE(val.has_value());
	REQUIRE((*val == "hello there \n\r\t 😀 ěščřž uff 😀→∑Δaabbccdde\\ĚŠČŘŽÝ😀😀"sv));

	BENCHMARK_ADVANCED("100B")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, 1));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("100kB")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("1MB")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, (1024 * 1024 / 100)));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("10MB")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, (10 * 1024 * 1024 / 100)));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("100B (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(100));
		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("100kB (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(100 * 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("1MB (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(1024 * 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("10MB (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(10 * 1024 * 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};
}

TEST_CASE("basics (branch)") {
	auto normalize = [](std::string & content) {
		auto reader = json::string_reader(content);
		const auto view = json::read_and_normalize_string<false>(reader);
		return view;
	};

	auto r = generate_random_json_string_with_length(100);
	REQUIRE(r.size() == 100u);
	const auto val0 = normalize(r);
	REQUIRE(val0.has_value());

	constexpr auto in = R"(hello there \n\r\t \uD83D\uDE00 ěščřž uff 😀\u2192\u2211\u0394aabbccdde\\ĚŠČŘŽÝ😀😀)"sv;
	REQUIRE(in.size() == 100u);

	std::string str = "\"" + std::string{in} + "\"";
	const auto val = normalize(str);
	REQUIRE(val.has_value());
	REQUIRE((*val == "hello there \n\r\t 😀 ěščřž uff 😀→∑Δaabbccdde\\ĚŠČŘŽÝ😀😀"sv));

	BENCHMARK_ADVANCED("100B")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, 1));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("100kB")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("1MB")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, (1024 * 1024 / 100)));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("10MB")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), repeat(in, (10 * 1024 * 1024 / 100)));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("100B (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(100));
		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("100kB (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(100 * 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("1MB (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(1024 * 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};

	BENCHMARK_ADVANCED("10MB (random)")
	(Catch::Benchmark::Chronometer meter) {
		std::vector<std::string> v(meter.runs());
		std::fill(v.begin(), v.end(), generate_random_json_string_with_length(10 * 1024 * 1024));

		meter.measure([&](int i) {
			const auto out = normalize(v[i]);
			REQUIRE(out.has_value());
			return out;
		});
	};
}