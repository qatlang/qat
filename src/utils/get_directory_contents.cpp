#include "./get_directory_contents.hpp"

namespace qat::utils {

Vec<fs::path> qatget_directory_contents(fs::path path, bool recursive) {
  Vec<fs::path> result;
  for (auto item : fs::directory_iterator(path)) {
    if (fs::is_directory(item)) {
      if (recursive) {
        auto contents = get_directory_contents(item, recursive);
        for (auto sub_item : contents) {
          result.push_back(sub_item);
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

} // namespace qat::utils