#ifndef QAT_UTILS_UTILS_HPP
#define QAT_UTILS_UTILS_HPP

#include "./helpers.hpp"
#include "./macros.hpp"

#include <array>
#include <random>

namespace qat::utils {

inline bool is_integer(StringView value) {
	for (auto ch : value) {
		if ((ch < 48) || (ch > 57)) {
			return false;
		}
	}
	return true;
}

inline String number_to_position(u64 number) {
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
}

inline Vec<String> split_string(const String& value, const String& slice) {
	Vec<String> result;
	usize       index = 0;
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

inline u64 random_number() {
	std::random_device                                          dev;
	std::mt19937_64                                             rng(dev());
	std::uniform_int_distribution<std::mt19937_64::result_type> dist(1, UINT_FAST64_MAX);
	return dist(rng);
}

inline Maybe<Pair<std::array<u8, 4>, u8>> unicode_scalar_to_utf8(u32 scalar) {
	std::array<u8, 4> bytes = {0, 0, 0, 0};
	if (scalar >= 0x0000 && scalar <= 0x007F) {
		bytes[0] = (u8)scalar;
		return std::make_pair(bytes, 1);
	} else if (scalar >= 0x0080 && scalar <= 0x07FF) {
		bytes[0] = 0b11000000 | static_cast<u8>(scalar >> 6);
		bytes[1] = 0b10000000 | (static_cast<u8>(scalar) & 0b00111111);
		return std::make_pair(bytes, 2);
	} else if (scalar >= 0x0800 && scalar <= 0xFFFF) {
		bytes[0] = 0b11100000 | (static_cast<u8>(scalar >> 12) & 0b00001111);
		bytes[1] = 0b10000000 | (static_cast<u8>(scalar >> 6) & 0b00111111);
		bytes[2] = 0b10000000 | (static_cast<u8>(scalar) & 0b00111111);
		return std::make_pair(bytes, 3);
	} else if (scalar >= 0x10000 && scalar <= 0x10FFFF) {
		bytes[0] = 0b11110000 | (static_cast<u8>(scalar >> 18) & 0b00000111);
		bytes[1] = 0b10000000 | (static_cast<u8>(scalar >> 12) & 0b00111111);
		bytes[2] = 0b10000000 | (static_cast<u8>(scalar >> 6) & 0b00111111);
		bytes[3] = 0b10000000 | (static_cast<u8>(scalar) & 0b00111111);
		return std::make_pair(bytes, 4);
	} else {
		return None;
	}
}

inline Maybe<u32> utf8_to_unicode_scalar(std::array<u8, 4> bytes) {
	u32 value = 0;
	if ((bytes[0] & 0x80) == 0) {
		value = bytes[0];
	} else if ((bytes[0] & 0xE0) == 0xC0) {
		value = ((u32(bytes[0] & 0x1F)) << 6) | u32(bytes[1] & 0x3F);
	} else if ((bytes[0] & 0xF0) == 0xE0) {
		value = (u32(bytes[0] & 0x0F) << 12) | (u32(bytes[1] & 0x3F) << 6) | u32(bytes[2] & 0x3F);
	} else if ((bytes[0] & 0xF8) == 0xF0) {
		value = (u32(bytes[0] & 0x07) << 18) | (u32(bytes[1] & 0x3F) << 12) | (u32(bytes[2] & 0x3F) << 6) |
		        u32(bytes[3] & 0x3F);
	} else {
		return None;
	}
	return value;
}

bool is_invisible_unicode(u32 scalar);

String to_hex_with_prefix(u32 value, Maybe<u8> width);

String to_hex(u32 value, Maybe<u8> width);

inline Maybe<u8> get_utf8_byte_length(u8 byte) {
	if ((byte & 0x80) == 0) {
		return 1;
	} else if ((byte & 0xE0) == 0xC0) {
		return 2;
	} else if ((byte & 0xF0) == 0xE0) {
		return 3;
	} else if ((byte & 0xF8) == 0xF0) {
		return 4;
	} else {
		return None;
	}
}

inline bool is_follow_byte_utf8(u8 byte) { return (byte & 0xC0) == 0x80; }

bool is_unicode_scalar_letter(u32 scalar);

bool is_unicode_scalar_digit(u32 scalar);

usize count_unicode_characters(String const& value);

bool bytewise_comparison(String const& first, String const& second);

} // namespace qat::utils

#endif
