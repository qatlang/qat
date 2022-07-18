#include "./function.hpp"

namespace qat {
namespace AST {

ArgumentType::ArgumentType(QatType *_type) : type(_type) {}

ArgumentType::ArgumentType(std::string _name, QatType *_type)
    : name(_name), type(_type) {}

bool ArgumentType::hasName() const { return name.has_value(); }

std::string ArgumentType::getName() const { return name.value_or(""); }

QatType *ArgumentType::getType() { return type; }

nuo::Json ArgumentType::toJson() const {
  return nuo::Json()
      ._("type", type->toJson())
      ._("hasName", name.has_value())
      ._("name", name.value_or(""));
}

FunctionType::FunctionType(QatType *_retType,
                           std::vector<ArgumentType *> _argTypes,
                           utils::FilePlacement _placement)
    : returnType(_retType), argTypes(_argTypes), QatType(false, _placement) {}

TypeKind FunctionType::typeKind() const { return TypeKind::function; }

nuo::Json FunctionType::toJson() const {
  std::vector<nuo::JsonValue> args;
  for (auto argTy : argTypes) {
    args.push_back(argTy->toJson());
  }
  return nuo::Json()
      ._("typeKind", "function")
      ._("returnType", returnType->toJson())
      ._("arguments", args);
}

} // namespace AST
} // namespace qat