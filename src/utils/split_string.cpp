#include "split_string.hpp"

namespace qat::utils {

Vec<String> splitString(const String &value, const String &slice) {
  Vec<String> result;
  int         index = 0;
  while (index < value.length()) {
    auto pos = value.find(slice, index);
    if (pos != String::npos) {
      result.push_back(value.substr(index, (pos - index)));
      index = pos + slice.length();
    } else {
      if (index < value.length()) {
        result.push_back(value.substr(index));
      }
      break;
    }
  }
  return result;
}

} // namespace qat::utils