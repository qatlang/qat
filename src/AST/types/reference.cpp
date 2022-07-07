#include "./reference.hpp"

namespace qat {
namespace AST {

ReferenceType::ReferenceType(QatType *_type, bool _variable,
                             utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

llvm::Type *ReferenceType::emit(IR::Generator *generator) {
  return llvm::PointerType::get(type->emit(generator), 0);
}

void ReferenceType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  type->emitCPP(file, isHeader);
  if (isConstant()) {
    file += " const ";
  }
  file += "& ";
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