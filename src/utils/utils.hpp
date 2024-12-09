#ifndef QAT_UTILS_UTILS_HPP
#define QAT_UTILS_UTILS_HPP

#include "./helpers.hpp"
#include "./macros.hpp"
#include <random>

namespace qat::utils {

useit inline bool is_integer(String value) {
	for (auto ch : value) {
		if ((ch < 48) || (ch > 57)) {
			return false;
		}
	}
	return true;
}

useit inline String number_to_position(u64 number) {
	// NOLINTBEGIN(readability-magic-numbers)
	if (((number % 100) >= 11) && ((number % 100) <= 20)) {
		return std::to_string(number) + "th";
	}
	switch (number % 10u) {
		case 1:
			return std::to_string(number).append("st");
		case 2:
			return std::to_string(number).append("nd");
		case 3:
			return std::to_string(number).append("rd");
		default:
			return std::to_string(number).append("th");
	}
	// NOLINTEND(readability-magic-numbers)
}

useit inline Vec<String> split_string(const String& value, const String& slice) {
	Vec<String> result;
	int         index = 0;
	while (index < value.length()) {
		auto pos = value.find(slice, index);
		if (pos != String::npos) {
			result.push_back(value.substr(index, (pos - index)));
			index = pos + slice.length();
		} else {
			if (index < value.length()) {
				result.push_back(value.substr(index));
			}
			break;
		}
	}
	return result;
}

useit inline u64 random_number() {
	std::random_device                                          dev;
	std::mt19937_64                                             rng(dev());
	std::uniform_int_distribution<std::mt19937_64::result_type> dist(1, UINT_FAST64_MAX);
	return dist(rng);
}

} // namespace qat::utils

#endif