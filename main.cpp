#include "normalize.hpp"

template <size_t Extent> bool test(const char (&in)[Extent]) {
	auto buffer = std::to_array(in);
	auto reader = string_reader(buffer);
	auto result = read_and_normalize_string(reader);
	if (!result) {
		std::cout << "[wrong input!]\n";
		return false;
	}
	std::cout << *result << "\n";
	return true;
}

int main() {
	test(R"("aloha")");
	test(R"("new\nline")");
	test(R"("back\\slash")");
	test(R"("quote\"quote")");
	test(R"("unicode\u0058unicode")");
	test(R"("unicode\u00E1two bytes")");
	test(R"("unicode\u0B83three bytes")");
	test(R"("emoji\uD83D\uDE00surrogate")");
	test(R"("surrogate\uD800\uDC00pair")");
}
