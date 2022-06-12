#ifndef QAT_UTILS_UNIQUE_ID_HPP
#define QAT_UTILS_UNIQUE_ID_HPP

#include <cstdint>
#include <random>
#include <sstream>
#include <string>

namespace qat {
namespace utils {

/**
 * @brief Generate a unique id. This is almost UUID
 *
 * @return std::string The Unique ID
 */
std::string unique_id();

} // namespace utils
} // namespace qat

#endif