#include "generate.hpp"
#include <iostream>

std::optional<std::string_view> normalize(std::string & content) {
	auto reader = json::string_reader(content);
	const auto view = json::read_and_normalize_string(reader);
	return view;
}

template <size_t Extent> bool test(const char (&in)[Extent]) {
	auto buffer = std::to_array(in);
	auto reader = json::string_reader(buffer);
	auto result = json::read_and_normalize_string(reader);
	if (!result) {
		std::cout << "[wrong input!]\n";
		return false;
	}
	std::cout << *result << "\n";
	return true;
}

int main() {
	test(R"("Mil\u00E1nek \uD83D\uDC68\uD83C\uDFFB\u200D\uD83C\uDF7C & Rob\u00EDnek\uD83D\uDC76")");

	auto in = generate_random_json_string_with_length(100);
	std::cout << in << "\n";

	for (int i = 0; i != 10000; ++i) {
		auto in = generate_random_json_string_with_length(1000);
		auto copy = in;
		// std::cout << in << "\n";
		try {
			const auto out = normalize(in);
			if (!out) {
				std::cout << "invalid input!\n";
				return 1;
			}
		} catch (const std::invalid_argument & inv) {
			std::cout << inv.what() << "\n";
			std::cout << copy << "\n";
			return 1;
		}

		// std::cout << *out << "\n";
	}
}
