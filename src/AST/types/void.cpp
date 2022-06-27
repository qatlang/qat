#include "./void.hpp"

namespace qat {
namespace AST {

VoidType::VoidType(const bool _variable,
                   const utils::FilePlacement _filePlacement)
    : QatType(_variable, _filePlacement) {}

llvm::Type *VoidType::generate(IR::Generator *generator) {
  return llvm::Type::getVoidTy(generator->llvmContext);
}

TypeKind VoidType::typeKind() { return TypeKind::Void; }

backend::JSON VoidType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "void")
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat