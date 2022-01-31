#ifndef QAT_UTILITIES_SPACE_POSSIBILITIES_HPP
#define QAT_UTILITIES_SPACE_POSSIBILITIES_HPP

#include "./parse_spaces_identifier.hpp"
#include <string>
#include <vector>

namespace qat {
namespace utilities {
std::vector<std::string> spacePossibilities(std::string value);
}
} // namespace qat

#endif