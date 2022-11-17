#include "./function.hpp"

namespace qat::ast {

ArgumentType::ArgumentType(QatType* _type) : type(_type) {}

ArgumentType::ArgumentType(String _name, QatType* _type) : name(_name), type(_type) {}

bool ArgumentType::hasName() const { return name.has_value(); }

String ArgumentType::getName() const { return name.value_or(""); }

QatType* ArgumentType::getType() { return type; }

Json ArgumentType::toJson() const {
  return Json()._("type", type->toJson())._("hasName", name.has_value())._("name", name.value_or(""));
}

FunctionType::FunctionType(QatType* _retType, Vec<ArgumentType*> _argTypes, utils::FileRange _fileRange)
    : QatType(false, _fileRange), returnType(_retType), argTypes(_argTypes) {}

FunctionType::~FunctionType() {
  for (auto* argTy : argTypes) {
    delete argTy;
  }
}

TypeKind FunctionType::typeKind() const { return TypeKind::function; }

Json FunctionType::toJson() const {
  Vec<JsonValue> args;
  for (auto argTy : argTypes) {
    args.push_back(argTy->toJson());
  }
  return Json()._("typeKind", "function")._("returnType", returnType->toJson())._("arguments", args);
}

} // namespace qat::ast