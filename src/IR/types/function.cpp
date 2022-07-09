#include "./function.hpp"

namespace qat {
namespace IR {

ArgumentType::ArgumentType(QatType *_type, bool _variability)
    : type(_type), variability(_variability) {}

QatType *ArgumentType::getType() { return type; }

bool ArgumentType::isVariable() { return variability; }

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

} // namespace IR
} // namespace qat