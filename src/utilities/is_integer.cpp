#include "./is_integer.hpp"

bool qat::utilities::isInteger(std::string value) {
  bool result = true;
  std::string digits = "0123456789";
  for (std::size_t i = 0; i < value.length(); i++) {
    if (digits.find_first_of(value.substr(i, 1)) == std::string::npos) {
      return false;
    }
  }
  return result;
}