#include "./parse_spaces_identifier.hpp"

std::vector<std::string>
qat::utilities::parseSpacesFromIdentifier(std::string fullname) {
  std::vector<std::string> spaces;
  int lastIndex = 0;
  for (std::size_t i = 0; i < fullname.length(); i++) {
    if (fullname[i] == ':') {
      spaces.push_back(fullname.substr(lastIndex, i - lastIndex));
      lastIndex = i + 1;
    }
  }
  return spaces;
}