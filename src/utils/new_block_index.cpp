#include "./new_block_index.hpp"

namespace qat {
namespace utils {

unsigned new_block_index(llvm::Function *fn) {
  auto index = 0;
  for (auto bb_i = fn->begin(); bb_i != fn->end(); ++bb_i) {
    auto bb_index = std::stoi(bb_i->getName().str());
    if (bb_index > index) {
      index = bb_index;
    }
  }
  return index + 1;
}

} // namespace utils
} // namespace qat