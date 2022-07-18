#ifndef QAT_UTILS_RANDOM_NUMBER_HPP
#define QAT_UTILS_RANDOM_NUMBER_HPP

#include <cstdint>
#include <random>
#include <sstream>
#include <string>

namespace qat::utils {

/**
 *  Generate a 64-bit random number with 64-bits of randomness
 *
 * @return uint64_t
 */
uint64_t random_number();

} // namespace qat::utils

#endif