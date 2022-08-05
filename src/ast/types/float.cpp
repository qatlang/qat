#include "./float.hpp"
#include "../../IR/types/float.hpp"

namespace qat::ast {

FloatType::FloatType(const IR::FloatTypeKind _kind, const bool _variable,
                     utils::FileRange _fileRange)
    : kind(_kind), QatType(_variable, std::move(_fileRange)) {}

IR::QatType *FloatType::emit(IR::Context *ctx) {
  return IR::FloatType::get(kind, ctx->llctx);
}

String FloatType::kindToString(IR::FloatTypeKind kind) {
  switch (kind) {
  case IR::FloatTypeKind::_half: {
    return "fhalf";
  }
  case IR::FloatTypeKind::_brain: {
    return "fbrain";
  }
  case IR::FloatTypeKind::_32: {
    return "f32";
  }
  case IR::FloatTypeKind::_64: {
    return "f64";
  }
  case IR::FloatTypeKind::_80: {
    return "f80";
  }
  case IR::FloatTypeKind::_128: {
    return "f128";
  }
  case IR::FloatTypeKind::_128PPC: {
    return "f128ppc";
  }
  }
}

TypeKind FloatType::typeKind() { return TypeKind::Float; }

nuo::Json FloatType::toJson() const {
  return nuo::Json()
      ._("typeKind", "float")
      ._("floatTypeKind", kindToString(kind))
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String FloatType::toString() const {
  return (isVariable() ? "var " : "") + kindToString(kind);
}

} // namespace qat::ast