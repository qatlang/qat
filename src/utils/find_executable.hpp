#ifndef QAT_UTILS_FIND_EXECUTABLE_HPP
#define QAT_UTILS_FIND_EXECUTABLE_HPP

#include "./helpers.hpp"

namespace qat {

Maybe<String> find_executable(StringView name);

inline bool check_executable_exists(StringView name) { return find_executable(name).has_value(); }

} // namespace qat

#endif
