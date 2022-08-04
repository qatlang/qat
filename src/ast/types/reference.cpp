#include "./reference.hpp"
#include "../../IR/types/reference.hpp"

namespace qat::ast {

ReferenceType::ReferenceType(QatType *_type, bool _variable,
                             utils::FileRange _fileRange)
    : type(_type), QatType(_variable, _fileRange) {}

IR::QatType *ReferenceType::emit(IR::Context *ctx) {
  return new IR::ReferenceType(type->emit(ctx));
}

TypeKind ReferenceType::typeKind() { return TypeKind::reference; }

nuo::Json ReferenceType::toJson() const {
  return nuo::Json()
      ._("typeKind", "reference")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String ReferenceType::toString() const {
  return (isVariable() ? "var @" : "@") + type->toString();
}

} // namespace qat::ast