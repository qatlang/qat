#ifndef QAT_UTILS_UNIQUE_ID_HPP
#define QAT_UTILS_UNIQUE_ID_HPP

#include <string>

namespace qat::utils {

/**
 *  Generate a unique id. This is almost UUID
 *
 * @return std::string The Unique ID
 */
std::string unique_id();

} // namespace qat::utils

#endif