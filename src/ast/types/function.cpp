#include "./function.hpp"
#include "../../IR/types/function.hpp"

namespace qat::ast {

void FunctionType::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> expect, IR::EntityState* ent,
                                       IR::Context* ctx) {
  returnType->update_dependencies(phase, IR::DependType::complete, ent, ctx);
  for (auto arg : argTypes) {
    arg->update_dependencies(phase, IR::DependType::complete, ent, ctx);
  }
}

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