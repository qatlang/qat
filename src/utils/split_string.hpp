#ifndef QAT_UTILS_SPLIT_STRING_HPP
#define QAT_UTILS_SPLIT_STRING_HPP

#include "helpers.hpp"

namespace qat::utils {

Vec<String> splitString(const String &value, const String &slice);

}

#endif