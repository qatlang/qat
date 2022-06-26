#include "./null_pointer.hpp"

namespace qat {
namespace AST {

NullPointer::NullPointer(utils::FilePlacement _filePlacement)
    : type(nullptr), Expression(_filePlacement) {}

NullPointer::NullPointer(llvm::Type *_type, utils::FilePlacement _filePlacement)
    : type(_type), Expression(_filePlacement) {}

void NullPointer::set_type(llvm::Type *type_val) { type = type_val; }

llvm::Value *NullPointer::generate(IR::Generator *generator) {
  if (getExpectedKind() == ExpressionKind::assignable) {
    generator->throw_error("This expression is not assignable", file_placement);
  }
  return llvm::Constant::getNullValue(
      type ? type->getPointerTo()
           : llvm::Type::getVoidTy(generator->llvmContext)->getPointerTo());
}

} // namespace AST
} // namespace qat