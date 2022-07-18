#include "./random_number.hpp"

namespace qat::utils {

uint64_t random_number() {
  std::random_device dev;
  std::mt19937_64 rng(dev());
  std::uniform_int_distribution<std::mt19937_64::result_type> dist(
      1, UINT_FAST64_MAX);
  return dist(rng);
}

} // namespace qat::utils