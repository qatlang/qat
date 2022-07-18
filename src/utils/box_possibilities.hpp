#ifndef QAT_UTILS_SPACE_POSSIBILITIES_HPP
#define QAT_UTILS_SPACE_POSSIBILITIES_HPP

#include "./parse_boxes_identifier.hpp"
#include <string>
#include <vector>

namespace qat::utils {

std::vector<std::string> boxPossibilities(std::string value);

} // namespace qat::utils

#endif