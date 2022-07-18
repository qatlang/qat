#include "./get_parent_names.hpp"
namespace qat::utils {

std::vector<std::string> get_parent_names(std::string name) {
  std::vector<std::string> result;
  for (std::size_t i = 0; i < name.length(); i++) {
    if (name.at(i) == ':') {
      result.push_back(name.substr(0, i));
    }
  }
  return result;
}

} // namespace qat::utils