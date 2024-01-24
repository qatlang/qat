#include "./function.hpp"
#include "../../IR/types/function.hpp"

namespace qat::ast {

// bool ArgumentType::hasName() const { return name.has_value(); }

// String ArgumentType::getName() const { return name.value_or(""); }

// QatType* ArgumentType::getType() { return type; }

// Json ArgumentType::toJson() const {
//   return Json()._("type", type->toJson())._("hasName", name.has_value())._("name", name.value_or(""));
// }

IR::QatType* FunctionType::emit(IR::Context* ctx) {
  Vec<IR::ArgumentType*> irArgTys;
  for (auto argTy : argTypes) {
    irArgTys.push_back(new IR::ArgumentType(argTy->emit(ctx), false));
  }
  IR::QatType* retTy = returnType->emit(ctx);
  return new IR::FunctionType(IR::ReturnType::get(retTy), irArgTys, ctx->llctx);
}

String FunctionType::toString() const {
  String result("(");
  for (usize i = 0; i < argTypes.size(); i++) {
    result.append(argTypes[i]->toString());
    if (i != (argTypes.size() - 1)) {
      result.append(", ");
    }
  }
  result += ") -> " + returnType->toString();
  return result;
}

Json FunctionType::toJson() const {
  Vec<JsonValue> args;
  for (auto argTy : argTypes) {
    args.push_back(argTy->toJson());
  }
  return Json()._("typeKind", "function")._("returnType", returnType->toJson())._("arguments", args);
}

} // namespace qat::ast