#include "./reference.hpp"

namespace qat {
namespace AST {

ReferenceType::ReferenceType(QatType *_type, bool _variable,
                             utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

llvm::Type *ReferenceType::generate(IR::Generator *generator) {
  return llvm::PointerType::get(type->generate(generator), 0);
}

TypeKind ReferenceType::typeKind() { return TypeKind::reference; }

backend::JSON ReferenceType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "reference")
      ._("subType", type->toJSON())
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat