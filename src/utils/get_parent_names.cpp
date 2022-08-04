#include "./get_parent_names.hpp"
namespace qat::utils {

Vec<String> get_parent_names(String name) {
  Vec<String> result;
  for (usize i = 0; i < name.length(); i++) {
    if (name.at(i) == ':') {
      result.push_back(name.substr(0, i));
    }
  }
  return result;
}

} // namespace qat::utils