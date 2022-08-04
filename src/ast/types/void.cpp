#include "../../IR/types/void.hpp"
#include "./void.hpp"

namespace qat::ast {

VoidType::VoidType(const bool _variable, const utils::FileRange _fileRange)
    : QatType(_variable, _fileRange) {}

IR::QatType *VoidType::emit(IR::Context *ctx) { return new IR::VoidType(); }

TypeKind VoidType::typeKind() { return TypeKind::Void; }

nuo::Json VoidType::toJson() const {
  return nuo::Json()
      ._("typeKind", "void")
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String VoidType::toString() const { return isVariable() ? "var void" : "void"; }

} // namespace qat::ast