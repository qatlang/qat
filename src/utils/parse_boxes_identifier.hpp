#ifndef QAT_UTILS_PARSE_SPACE_IDENTIFIER_HPP
#define QAT_UTILS_PARSE_SPACE_IDENTIFIER_HPP

#include <string>
#include <vector>

namespace qat::utils {

std::vector<std::string> parseSectionsFromIdentifier(std::string fullname);

} // namespace qat::utils

#endif