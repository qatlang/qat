#include "./tuple.hpp"
#include "../../IR/types/tuple.hpp"
#include <vector>

namespace qat::ast {

TupleType::TupleType(const Vec<ast::QatType *> _types, const bool _isPacked,
                     const bool _variable, const utils::FileRange _fileRange)
    : types(_types), isPacked(_isPacked), QatType(_variable, _fileRange) {}

IR::QatType *TupleType::emit(IR::Context *ctx) {
  Vec<IR::QatType *> irTypes;
  for (auto &type : types) {
    if (type->typeKind() == ast::TypeKind::Void) {
      ctx->Error("Tuple member type cannot be `void`", fileRange);
    }
    irTypes.push_back(type->emit(ctx));
  }
  return new IR::TupleType(irTypes, isPacked);
}

TypeKind TupleType::typeKind() { return TypeKind::tuple; }

nuo::Json TupleType::toJson() const {
  Vec<nuo::JsonValue> mems;
  for (auto mem : types) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()
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
      result += ", ";
    }
  }
  result += ")";
}

} // namespace qat::ast