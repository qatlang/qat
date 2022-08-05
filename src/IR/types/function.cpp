#include "./function.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ArgumentType::ArgumentType(QatType *_type, bool _variability)
    : type(_type), variability(_variability) {}

ArgumentType::ArgumentType(String _name, QatType *_type, bool _variability)
    : name(_name), type(_type), variability(_variability) {}

bool ArgumentType::hasName() const { return name.has_value(); }

String ArgumentType::getName() const { return name.value_or(""); }

QatType *ArgumentType::getType() { return type; }

bool ArgumentType::isVariable() const { return variability; }

String ArgumentType::toString() const {
  return type->toString() + (name.has_value() ? (" " + name.value()) : "");
}

FunctionType::FunctionType(QatType *_retType, bool _isRetTypeVariable,
                           Vec<ArgumentType *> _argTypes,
                           llvm::LLVMContext  &ctx)
    : returnType(_retType), isReturnVariable(_isRetTypeVariable),
      argTypes(std::move(_argTypes)) {
  Vec<llvm::Type *> argTys;
  for (auto *arg : argTypes) {
    argTys.push_back(arg->getType()->getLLVMType());
  }
  llvmType = llvm::FunctionType::get(returnType->getLLVMType(), argTys, false);
}

QatType *FunctionType::getReturnType() { return returnType; }

bool FunctionType::isReturnTypeVariable() const { return isReturnVariable; }

ArgumentType *FunctionType::getArgumentTypeAt(u32 index) {
  return argTypes.at(index);
}

Vec<ArgumentType *> FunctionType::getArgumentTypes() const { return argTypes; }

u64 FunctionType::getArgumentCount() const { return argTypes.size(); }

String FunctionType::toString() const {
  String result("(");
  for (usize i = 0; i < argTypes.size(); i++) {
    result += (argTypes.at(i)->toString());
    if (i != (argTypes.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
  result += " -> " + returnType->toString();
  return result;
}

nuo::Json FunctionType::toJson() const {
  Vec<nuo::JsonValue> vals;
  for (auto *arg : argTypes) {
    vals.push_back(arg->getType()->toJson());
  }
  return nuo::Json()
      ._("typeKind", "function")
      ._("returnType", returnType->getID())
      ._("argumentTypes", vals);
}

} // namespace qat::IR