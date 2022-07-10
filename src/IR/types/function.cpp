#include "./function.hpp"

namespace qat {
namespace IR {

ArgumentType::ArgumentType(QatType *_type, bool _variability)
    : type(_type), variability(_variability) {}

ArgumentType::ArgumentType(std::string _name, QatType *_type, bool _variability)
    : name(_name), type(_type), variability(_variability) {}

bool ArgumentType::hasName() const { return name.has_value(); }

std::string ArgumentType::getName() const { return name.value_or(""); }

QatType *ArgumentType::getType() { return type; }

bool ArgumentType::isVariable() const { return variability; }

std::string ArgumentType::toString() const {
  return type->toString() + (name.has_value() ? (" " + name.value()) : "");
}

FunctionType::FunctionType(QatType *_retType, bool _isRetTypeVariable,
                           std::vector<ArgumentType *> _argTypes)
    : returnType(_retType), isReturnVariable(_isRetTypeVariable),
      argTypes(_argTypes) {}

QatType *FunctionType::getReturnType() { return returnType; }

bool FunctionType::isReturnTypeVariable() const { return isReturnVariable; }

ArgumentType *FunctionType::getArgumentTypeAt(unsigned int index) {
  return argTypes.at(index);
}

std::vector<ArgumentType *> FunctionType::getArgumentTypes() const {
  return argTypes;
}

unsigned FunctionType::getArgumentCount() const { return argTypes.size(); }

std::string FunctionType::toString() const {
  std::string result("(");
  for (std::size_t i = 0; i < argTypes.size(); i++) {
    result += (argTypes.at(i)->toString());
    if (i != (argTypes.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
  result += " -> " + returnType->toString();
}

} // namespace IR
} // namespace qat