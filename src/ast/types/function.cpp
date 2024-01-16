#include "./function.hpp"

namespace qat::ast {

bool ArgumentType::hasName() const { return name.has_value(); }

String ArgumentType::getName() const { return name.value_or(""); }

QatType* ArgumentType::getType() { return type; }

Json ArgumentType::toJson() const {
  return Json()._("type", type->toJson())._("hasName", name.has_value())._("name", name.value_or(""));
}

FunctionType::~FunctionType() {
  for (auto* argTy : argTypes) {
    std::destroy_at(argTy);
  }
}

AstTypeKind FunctionType::typeKind() const { return AstTypeKind::FUNCTION; }

Json FunctionType::toJson() const {
  Vec<JsonValue> args;
  for (auto argTy : argTypes) {
    args.push_back(argTy->toJson());
  }
  return Json()._("typeKind", "function")._("returnType", returnType->toJson())._("arguments", args);
}

} // namespace qat::ast