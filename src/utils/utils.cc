#include "./utils.hpp"
#include "./find_executable.hpp"
#include "./run_command.hpp"
#include "./unique_id.hpp"

#include <cstdio>
#include <random>
#include <set>
#include <unicode/brkiter.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/ustring.h>

#if MINGW_RUNTIME
#include <sdkddkver.h>
#include <windows.h>
#elif MSVC_RUNTIME
#include <SDKDKKVer.h>
#include <Windows.h>
#endif

namespace qat {

namespace utils {

u64 unique_id() {
	static u64 uniqueIDCounter = 0;
	return uniqueIDCounter++;
	// char               hex_vals[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E',
	// 'F'}; String             result; std::random_device dev1; std::random_device dev2; std::random_device dev3;
	// std::mt19937_64    rng1(dev1());
	// std::mt19937_64    rng2(dev2());
	// std::mt19937_64    rng3(dev3());
	// std::uniform_int_distribution<std::mt19937_64::result_type> dist1(0, 15);
	// std::uniform_int_distribution<std::mt19937_64::result_type> dist2(0, 15);
	// std::uniform_int_distribution<std::mt19937_64::result_type> switch_dist(0, 1);
	// for (usize i = 0; i < 32; i++) {
	// 	result += hex_vals[(switch_dist(rng3) == 1) ? dist2(rng2) : dist1(rng1)];
	// }
	// return result;
}

String uid_string() { return std::to_string(unique_id()); }

bool is_invisible_unicode(u32 scalar) {
	static std::set<u32> invisibles{0x200B, 0x200C, 0x200D, 0x2060, 0xFEFF, 0x00A0, 0x2000, 0x2001, 0x2002, 0x2003,
	                                0x2004, 0x2005, 0x2006, 0x2007, 0x2008, 0x2009, 0x200A, 0x202F, 0x205F, 0x202A,
	                                0x202B, 0x202C, 0x202D, 0x202E, 0x2066, 0x2067, 0x2068, 0x2069, 0x200E, 0x200F};
	return invisibles.contains(scalar);
}

String to_hex_with_prefix(u32 value, Maybe<u8> width) { return "0x" + to_hex(value, width); }

String to_hex(u32 value, Maybe<u8> width) {
	std::stringstream ss;
	ss << std::hex << std::uppercase << std::setfill('0');
	if (width.has_value()) {
		ss << std::setw(width.value());
	}
	ss << value;
	return ss.str();
}

bool is_unicode_scalar_letter(u32 scalar) {
	switch (u_charType_74(scalar)) {
		case U_UPPERCASE_LETTER:
		case U_LOWERCASE_LETTER:
		case U_TITLECASE_LETTER:
			return true;
		default:
			return false;
	}
}

bool is_unicode_scalar_digit(u32 scalar) {
	switch (u_charType_74(scalar)) {
		case U_DECIMAL_DIGIT_NUMBER:
			return true;
		default:
			return false;
	}
}

usize count_unicode_characters(String const& value) {
	auto                                   str       = icu_74::UnicodeString::fromUTF8(value);
	UErrorCode                             errorCode = U_ZERO_ERROR;
	std::unique_ptr<icu_74::BreakIterator> iter(
	    icu_74::BreakIterator::createCharacterInstance(icu_74::Locale::getDefault(), errorCode));
	if (U_SUCCESS(errorCode)) {
		iter->setText(str);
		usize count = 0;
		while (iter->next() != icu_74::BreakIterator::DONE) {
			count++;
		}
		return count;
	} else {
		return value.length();
	}
}

} // namespace utils

Maybe<String> find_executable(StringView name) {
	const StringView path = std::getenv("PATH");
#if OS_IS_WINDOWS
	const StringView pathExt = std::getenv("PATHENV");

	Vec<StringView> extensions(15);
	usize           i = 0;
	while (i < pathExt.size()) {
		auto sep = pathExt.find_first_of(';', i);
		if (sep != StringView::npos) {
			extensions.push_back(pathExt.substr(i, sep - i));
		} else {
			extensions.push_back(pathExt.substr(i));
		}
		if (sep != StringView::npos) {
			i = sep + 1;
		} else {
			i = pathExt.size();
		}
	}

	i = 0;
	while (i < path.size()) {
		fs::path it;
		auto     sep = path.find_first_of(';', i);
		if (sep != StringView::npos) {
			it = path.substr(i, sep - i);
			i  = sep + 1;
		} else {
			it = path.substr(i);
			i  = path.size();
		}
		for (auto& ext : extensions) {
			auto cand = it / (String(name).append(ext));
			if (fs::exists(cand)) {
				return cand.string();
			}
		}
	}
	if (name != "where") {
		auto wherePath = find_executable("where");
		if (wherePath.has_value()) {
			auto res = run_command_get_output(wherePath.value(), {String(name)});
			if (not res.has_value()) {
				return None;
			}
			if (res->first == 0) {
				if (res->second.ends_with('\n')) {
					res->second.pop_back();
				}
				if (fs::is_regular_file(res->second)) {
					return res->second;
				}
			}
		}
	}
	return None;
#else
	usize i = 0;
	while (i < path.size()) {
		fs::path it;
		auto     colon = path.find_first_of(':', i);
		if (colon != StringView::npos) {
			it = path.substr(i, colon - i);
			i  = colon + 1;
		} else {
			it = path.substr(i);
			i  = path.size();
		}
		auto cand = it / name;
		if (fs::exists(cand)) {
			return cand.string();
		}
	}
	if (name != "which") {
		auto whichPath = find_executable("which");
		if (whichPath.has_value()) {
			auto res = run_command_get_output(whichPath.value(), {String(name)});
			if (res.first == 0) {
				if (res.second.ends_with('\n')) {
					res.second.pop_back();
				}
				if (fs::is_regular_file(res.second)) {
					return res.second;
				}
			}
		}
	}
	return None;
#endif
}

} // namespace qat
