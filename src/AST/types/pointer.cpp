#include "./pointer.hpp"
#include "../../show.hpp"

namespace qat {
namespace AST {

PointerType::PointerType(QatType *_type, const bool _variable,
                         const utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

llvm::Type *PointerType::emit(IR::Generator *generator) {
  SHOW("Before generating type of Pointer")
  auto genType = type->emit(generator)->getPointerTo();
  SHOW("Generated type " << utils::llvmTypeToName(genType))
  return genType;
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