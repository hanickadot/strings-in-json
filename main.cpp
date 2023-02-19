#include "generate.hpp"
#include <iostream>

std::optional<std::string_view> normalize(std::string & content) {
	auto reader = json::string_reader(content);
	const auto view = json::read_and_normalize_string(reader);
	return view;
}

int main() {
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
