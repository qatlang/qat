#include "./tuple.hpp"
#include "../../IR/types/tuple.hpp"
#include <vector>

namespace qat::ast {

Maybe<usize> TupleType::getTypeSizeInBits(IR::Context* ctx) const {
  usize total = 0;
  for (auto* typ : types) {
    auto typSiz = typ->getTypeSizeInBits(ctx);
    if (typSiz.has_value()) {
      total += typSiz.value();
    } else {
      return None;
    }
  }
  return total;
}

IR::QatType* TupleType::emit(IR::Context* ctx) {
  Vec<IR::QatType*> irTypes;
  for (auto* type : types) {
    if (type->typeKind() == ast::AstTypeKind::VOID) {
      ctx->Error("Tuple cannot contain a member of type " + ctx->highlightError("void"), fileRange);
    }
    irTypes.push_back(type->emit(ctx));
  }
  return IR::TupleType::get(irTypes, isPacked, ctx->llctx);
}

AstTypeKind TupleType::typeKind() const { return AstTypeKind::TUPLE; }

Json TupleType::toJson() const {
  Vec<JsonValue> mems;
  for (auto* mem : types) {
    mems.push_back(mem->toJson());
  }
  return Json()._("typeKind", "tuple")._("members", mems)._("isPacked", isPacked)._("fileRange", fileRange);
}

String TupleType::toString() const {
  String result;
  if (isPacked) {
    result += "pack ";
  }
  result = "(";
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