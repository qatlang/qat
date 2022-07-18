#include "../../IR/types/void.hpp"
#include "./void.hpp"

namespace qat::AST {

VoidType::VoidType(const bool _variable, const utils::FileRange _filePlacement)
    : QatType(_variable, _filePlacement) {}

IR::QatType *VoidType::emit(IR::Context *ctx) { return new IR::VoidType(); }

void VoidType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file += "void ";
}

TypeKind VoidType::typeKind() { return TypeKind::Void; }

nuo::Json VoidType::toJson() const {
  return nuo::Json()
      ._("typeKind", "void")
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

std::string VoidType::toString() const {
  return isVariable() ? "var void" : "void";
}

} // namespace qat::AST