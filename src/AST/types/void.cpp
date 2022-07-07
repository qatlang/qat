#include "./void.hpp"

namespace qat {
namespace AST {

VoidType::VoidType(const bool _variable,
                   const utils::FilePlacement _filePlacement)
    : QatType(_variable, _filePlacement) {}

llvm::Type *VoidType::emit(IR::Generator *generator) {
  return llvm::Type::getVoidTy(generator->llvmContext);
}

void VoidType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file += "void ";
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