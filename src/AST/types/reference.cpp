#include "./reference.hpp"
#include "../../IR/types/reference.hpp"

namespace qat::AST {

ReferenceType::ReferenceType(QatType *_type, bool _variable,
                             utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

IR::QatType *ReferenceType::emit(IR::Context *ctx) {
  return new IR::ReferenceType(type->emit(ctx));
}

void ReferenceType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  type->emitCPP(file, isHeader);
  if (isConstant()) {
    file += " const ";
  }
  file += "& ";
}

TypeKind ReferenceType::typeKind() { return TypeKind::reference; }

nuo::Json ReferenceType::toJson() const {
  return nuo::Json()
      ._("typeKind", "reference")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

std::string ReferenceType::toString() const {
  return (isVariable() ? "var @" : "@") + type->toString();
}

} // namespace qat::AST