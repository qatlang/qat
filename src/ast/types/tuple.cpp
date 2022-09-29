#include "./tuple.hpp"
#include "../../IR/types/tuple.hpp"
#include <vector>

namespace qat::ast {

TupleType::TupleType(Vec<ast::QatType *> _types, bool _isPacked, bool _variable,
                     utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), types(std::move(_types)),
      isPacked(_isPacked) {}

IR::QatType *TupleType::emit(IR::Context *ctx) {
  Vec<IR::QatType *> irTypes;
  for (auto *type : types) {
    if (type->typeKind() == ast::TypeKind::Void) {
      ctx->Error("Tuple member type cannot be `void`", fileRange);
    }
    irTypes.push_back(type->emit(ctx));
  }
  return IR::TupleType::get(irTypes, isPacked, ctx->llctx);
}

TypeKind TupleType::typeKind() const { return TypeKind::tuple; }

Json TupleType::toJson() const {
  Vec<JsonValue> mems;
  for (auto *mem : types) {
    mems.push_back(mem->toJson());
  }
  return Json()
      ._("typeKind", "tuple")
      ._("members", mems)
      ._("isVariable", isVariable())
      ._("isPacked", isPacked)
      ._("fileRange", fileRange);
}

String TupleType::toString() const {
  String result;
  if (isVariable()) {
    result = "var (";
  } else {
    result = "(";
  }
  for (usize i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += "; ";
    }
  }
  result += ")";
  return result;
}

} // namespace qat::ast