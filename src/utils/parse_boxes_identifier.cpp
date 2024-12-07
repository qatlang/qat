#include "./parse_boxes_identifier.hpp"

namespace qat::utils {

Vec<String> parseSectionsFromIdentifier(String fullname) {
	Vec<String> spaces;
	usize		lastIndex = 0;
	for (usize i = 0; i < fullname.length(); i++) {
		if (fullname[i] == ':') {
			spaces.push_back(fullname.substr(lastIndex, i - lastIndex));
			lastIndex = i + 1;
		}
	}
	return spaces;
}

} // namespace qat::utils