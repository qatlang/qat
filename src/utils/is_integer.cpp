#include "./is_integer.hpp"

namespace qat::utils {

bool isInteger(const String &value) {
  bool   result = true;
  String digits = "0123456789";
  for (usize i = 0; i < value.length(); i++) {
    if (digits.find_first_of(value.substr(i, 1)) == String::npos) {
      return false;
    }
  }
  return result;
}

} // namespace qat::utils