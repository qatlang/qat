#include "./directory_contents.hpp"

std::vector<fsexp::path> qat::utilities::directoryContents(fsexp::path path,
                                                           bool recursive) {
  std::vector<fsexp::path> result;
  for (auto item : fsexp::directory_iterator(path)) {
    if (fsexp::is_directory(item)) {
      if (recursive) {
        auto subItems = directoryContents(item, recursive);
        for (auto subItem : subItems) {
          result.push_back(subItem);
        }
      } else {
        result.push_back(item.path());
      }
    } else {
      result.push_back(item.path());
    }
  }
  return result;
}