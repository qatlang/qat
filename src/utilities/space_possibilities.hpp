#ifndef QAT_UTILITIES_SPACE_POSSIBILITIES_HPP
#define QAT_UTILITIES_SPACE_POSSIBILITIES_HPP

#include <vector>
#include <string>
#include "./parse_spaces_identifier.hpp"

namespace qat{
    namespace utilities {
        std::vector<std::string> spacePossibilities(std::string value);
    }
}

#endif