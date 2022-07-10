#include "./function.hpp"

namespace qat {
namespace AST {

ArgumentType::ArgumentType(QatType *_type) : type(_type) {}

ArgumentType::ArgumentType(std::string _name, QatType *_type)
    : name(_name), type(_type) {}

bool ArgumentType::hasName() const { return name.has_value(); }

std::string ArgumentType::getName() const { return name.value_or(""); }

QatType *ArgumentType::getType() { return type; }

backend::JSON ArgumentType::toJSON() const {
  return backend::JSON()
      ._("type", type->toJSON())
      ._("hasName", name.has_value())
      ._("name", name.value_or(""));
}

FunctionType::FunctionType(QatType *_retType,
                           std::vector<ArgumentType *> _argTypes,
                           utils::FilePlacement _placement)
    : returnType(_retType), argTypes(_argTypes), QatType(false, _placement) {}

TypeKind FunctionType::typeKind() const { return TypeKind::function; }

backend::JSON FunctionType::toJSON() const {
  std::vector<backend::JSON> args;
  for (auto argTy : argTypes) {
    args.push_back(argTy->toJSON());
  }
  return backend::JSON()
      ._("typeKind", "function")
      ._("returnType", returnType->toJSON())
      ._("arguments", args);
}

} // namespace AST
} // namespace qat