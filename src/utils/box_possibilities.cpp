#include "./box_possibilities.hpp"

namespace qat::utils {

Vec<String> boxPossibilities(String value) {
  auto parsed_values = parseSectionsFromIdentifier(value);
  if (parsed_values.size() > 1) {
    Vec<String> result;
    for (usize i = 0; i < parsed_values.size(); i++) {
      String candidate;
      for (usize j = 0; j <= i; j++) {
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