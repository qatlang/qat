#include "../../IR/types/cstring.hpp"
#include "./cstring.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

CStringType::CStringType(bool _variable, utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)) {}

IR::QatType *CStringType::emit(IR::Context *ctx) {
  return IR::CStringType::get(ctx->llctx);
}

TypeKind CStringType::typeKind() const { return TypeKind::cstring; }

nuo::Json CStringType::toJson() const {
  return nuo::Json()
      ._("typeKind", "cstring")
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String CStringType::toString() const {
  return isVariable() ? "var cstring" : "cstring";
}

} // namespace qat::ast