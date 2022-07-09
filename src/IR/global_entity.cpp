#include "./global_entity.hpp"
#include "./qat_module.hpp"
#include "llvm/Support/Casting.h"

namespace qat {
namespace IR {

std::string GlobalEntity::getName() const { return name; }

std::string GlobalEntity::getFullName() const {
  return parent->getFullName() + ":" + name;
}

const utils::VisibilityInfo &GlobalEntity::getVisibility() const {
  return visibility;
}

unsigned GlobalEntity::getLoadCount() const { return loads; }

unsigned GlobalEntity::getStoreCount() const { return stores; }

unsigned GlobalEntity::getReferCount() const { return refers; }

// llvm::Value *GlobalEntity::get(
//     llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>
//         &builder,
//     const bool as_reference, const utils::RequesterInfo &req_info) {
//   if (visibility.isAccessible(req_info)) {
//     if (llvm::isa<llvm::Constant>(getLLVMValue())) {
//       return getLLVMValue();
//     } else {
//       if (as_reference) {
//         return getLLVMValue();
//       } else {
//         return builder.CreateLoad(
//             llvm::dyn_cast<llvm::GlobalVariable>(getLLVMValue())
//                 ->getValueType(),
//             getLLVMValue(), fullName());
//       }
//     }
//   } else {
//     return nullptr;
//   }
// }

} // namespace IR
} // namespace qat