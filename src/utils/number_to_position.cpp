#include "./number_to_position.hpp"
#include <string>

namespace qat::utils {

String numberToPosition(u64 number) {
  // NOLINTNEXTLINE(readability-magic-numbers)
  switch (number % 10u) {
    case 1:
      return std::to_string(number).append("st");
    case 2:
      return std::to_string(number).append("nd");
    case 3:
      return std::to_string(number).append("rd");
    default:
      return std::to_string(number).append("th");
  }
}

} // namespace qat::utils