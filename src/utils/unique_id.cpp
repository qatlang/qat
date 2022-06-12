#include "./unique_id.hpp"

namespace qat {
namespace utils {

std::string unique_id() {
  char hex_vals[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
                       '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  std::string result;
  std::random_device dev1;
  std::random_device dev2;
  std::random_device dev3;
  std::mt19937_64 rng1(dev1());
  std::mt19937_64 rng2(dev2());
  std::mt19937_64 rng3(dev3());
  std::uniform_int_distribution<std::mt19937_64::result_type> dist1(0, 15);
  std::uniform_int_distribution<std::mt19937_64::result_type> dist2(0, 15);
  std::uniform_int_distribution<std::mt19937_64::result_type> switch_dist(0, 1);
  for (std::size_t i = 0; i < 32; i++) {
    result += hex_vals[(switch_dist(rng3) == 1) ? dist2(rng2) : dist1(rng1)];
  }
  return result;
}

} // namespace utils
} // namespace qat