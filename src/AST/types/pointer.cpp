#include "./pointer.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

namespace qat {
namespace AST {

PointerType::PointerType(QatType *_type, const bool _variable,
                         const utils::FilePlacement _filePlacement)
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

backend::JSON PointerType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "pointer")
      ._("subType", type->toJSON())
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat