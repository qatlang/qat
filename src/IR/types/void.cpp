#include "./void.hpp"

namespace qat {
namespace IR {

VoidType::VoidType(llvm::LLVMContext &llctx) {
  llvmType = llvm::Type::getVoidTy(llctx);
}

TypeKind VoidType::typeKind() const { return TypeKind::Void; }

} // namespace IR
} // namespace qat