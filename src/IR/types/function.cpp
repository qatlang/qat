#include "./function.hpp"
#include "../../show.hpp"
#include "./reference.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

ArgumentType::ArgumentType(QatType* _type, bool _variability)
    : type(_type), variability(_variability), isMemberArg(false) {}

ArgumentType::ArgumentType(String _name, QatType* _type, bool _variability)
    : name(_name), type(_type), variability(_variability), isMemberArg(false) {}

ArgumentType::ArgumentType(String _name, QatType* _type, bool _isMemberArg, bool _variability)
    : name(_name), type(_type), variability(_variability), isMemberArg(_isMemberArg) {}

bool ArgumentType::isMemberArgument() const { return isMemberArg; }

bool ArgumentType::hasName() const { return name.has_value(); }

String ArgumentType::getName() const { return name.value_or(""); }

QatType* ArgumentType::getType() { return type; }

bool ArgumentType::isVariable() const { return variability; }

String ArgumentType::toString() const { return type->toString() + (name.has_value() ? (" " + name.value()) : ""); }

ReturnType::ReturnType(QatType* _retTy, bool _isRetSelfRef) : retTy(_retTy), isReturnSelfRef(_isRetSelfRef) {}

ReturnType* ReturnType::get(QatType* _retTy) { return new ReturnType(_retTy, false); }

ReturnType* ReturnType::get(QatType* _retTy, bool _isRetSelf) { return new ReturnType(_retTy, _isRetSelf); }

QatType* ReturnType::getType() const { return retTy; }

bool ReturnType::isReturnSelf() const { return isReturnSelfRef; }

String ReturnType::toString() const { return isReturnSelfRef ? "''" : retTy->toString(); }

FunctionType::FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& llctx)
    : returnType(_retType), argTypes(std::move(_argTypes)) {
  SHOW("Creating function type")
  linkingName = "qat'fn_type:[(";
  for (usize i = 0; i < argTypes.size(); i++) {
    linkingName += argTypes.at(i)->getType()->getNameForLinking();
    if (i != (argTypes.size() - 1)) {
      linkingName += ", ";
    }
  }
  linkingName += ") -> " + returnType->getType()->getNameForLinking() + "]";
  Vec<llvm::Type*> argTys;
  for (auto* arg : argTypes) {
    SHOW("Arg name is " << arg->getName())
    if (arg->getType()->isReference()) {
      SHOW("Arg type is " << arg->getType()->asReference()->getSubType()->getLLVMType()->getTypeID())
    }
    argTys.push_back(arg->getType()->getLLVMType());
  }
  SHOW("Got arg llvm types in FunctionType")
  llvmType = llvm::FunctionType::get(returnType->getType()->getLLVMType(), argTys, false);
}

FunctionType::~FunctionType() {
  delete returnType;
  for (auto* argTy : argTypes) {
    delete argTy;
  }
}

ReturnType* FunctionType::getReturnType() { return returnType; }

ArgumentType* FunctionType::getArgumentTypeAt(u32 index) { return argTypes.at(index); }

Vec<ArgumentType*> FunctionType::getArgumentTypes() const { return argTypes; }

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

} // namespace qat::IR