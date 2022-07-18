#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

namespace qat::AST {

PointerType::PointerType(QatType *_type, const bool _variable,
                         const utils::FileRange _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

IR::QatType *PointerType::emit(IR::Context *ctx) {
  return new IR::PointerType(type->emit(ctx));
}

void PointerType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  type->emitCPP(file, isHeader);
  if (isConstant()) {
    file += " const";
  }
  file += "* ";
}

TypeKind PointerType::typeKind() { return TypeKind::pointer; }

nuo::Json PointerType::toJson() const {
  return nuo::Json()
      ._("typeKind", "pointer")
      ._("subType", type->toJson())
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

std::string PointerType::toString() const {
  return (isVariable() ? "var #[" : "#[") + type->toString() + "]";
}

} // namespace qat::AST