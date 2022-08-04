#ifndef QAT_UTILS_UNIQUE_ID_HPP
#define QAT_UTILS_UNIQUE_ID_HPP

#include "./helpers.hpp"

namespace qat::utils {

/**
 *  Generate a unique id. This is almost UUID
 *
 * @return String The Unique ID
 */
String unique_id();

} // namespace qat::utils

#endif