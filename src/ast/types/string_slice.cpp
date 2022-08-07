#include "./string_slice.hpp"
#include "../../IR/types/string_slice.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

StringSliceType::StringSliceType(bool _variable, utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)) {}

IR::QatType *StringSliceType::emit(IR::Context *ctx) {
  return IR::StringSliceType::get(ctx->llctx);
}

TypeKind StringSliceType::typeKind() const { return TypeKind::stringSlice; }

nuo::Json StringSliceType::toJson() const {
  return nuo::Json()
      ._("typeKind", "stringSlice")
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String StringSliceType::toString() const {
  return isVariable() ? "var str" : "str";
}

} // namespace qat::ast