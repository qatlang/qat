#include "./box_possibilities.hpp"

namespace qat::utils {

std::vector<std::string> boxPossibilities(std::string value) {
  auto parsed_values = parseSectionsFromIdentifier(value);
  if (parsed_values.size() > 1) {
    std::vector<std::string> result;
    for (std::size_t i = 0; i < parsed_values.size(); i++) {
      std::string candidate;
      for (std::size_t j = 0; j <= i; j++) {
        candidate += parsed_values.at(j);
        candidate += ':';
      }
      result.push_back(candidate);
    }
    return result;
  } else {
    return parsed_values;
  }
}

} // namespace qat::utils