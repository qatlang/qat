#include "./function.hpp"
#include "../../show.hpp"
#include "./reference.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

ArgumentType::ArgumentType(Type* _type, bool _variability)
    : type(_type), variability(_variability), isMemberArg(false) {}

ArgumentType::ArgumentType(String _name, Type* _type, bool _variability)
    : name(_name), type(_type), variability(_variability), isMemberArg(false) {}

ArgumentType::ArgumentType(String _name, Type* _type, bool _is_member_argument, bool _variability)
    : name(_name), type(_type), variability(_variability), isMemberArg(_is_member_argument) {}

bool ArgumentType::is_member_argument() const { return isMemberArg; }

bool ArgumentType::has_name() const { return name.has_value(); }

String ArgumentType::get_name() const { return name.value_or(""); }

Type* ArgumentType::get_type() { return type; }

bool ArgumentType::is_variable() const { return variability; }

String ArgumentType::to_string() const { return type->to_string() + (name.has_value() ? (" " + name.value()) : ""); }

ReturnType::ReturnType(Type* _retTy, bool _isRetSelfRef) : retTy(_retTy), isReturnSelfRef(_isRetSelfRef) {}

ReturnType* ReturnType::get(Type* _retTy) { return new ReturnType(_retTy, false); }

ReturnType* ReturnType::get(Type* _retTy, bool _isRetSelf) { return new ReturnType(_retTy, _isRetSelf); }

Type* ReturnType::get_type() const { return retTy; }

bool ReturnType::is_return_self() const { return isReturnSelfRef; }

String ReturnType::to_string() const { return isReturnSelfRef ? "''" : retTy->to_string(); }

FunctionType::FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& llctx)
    : returnType(_retType), argTypes(std::move(_argTypes)) {
  SHOW("Creating function type")
  linkingName = "qat'fn_type:[(";
  for (usize i = 0; i < argTypes.size(); i++) {
    linkingName += argTypes.at(i)->get_type()->get_name_for_linking();
    if (i != (argTypes.size() - 1)) {
      linkingName += ", ";
    }
  }
  linkingName += ") -> " + returnType->get_type()->get_name_for_linking() + "]";
  Vec<llvm::Type*> argTys;
  for (auto* arg : argTypes) {
    SHOW("Arg name is " << arg->get_name())
    if (arg->get_type()->is_reference()) {
      SHOW("Arg type is " << arg->get_type()->as_reference()->get_subtype()->get_llvm_type()->getTypeID())
    }
    argTys.push_back(arg->get_type()->get_llvm_type());
  }
  SHOW("Got arg llvm types in FunctionType")
  llvmType = llvm::FunctionType::get(returnType->get_type()->get_llvm_type(), argTys, false);
}

FunctionType::~FunctionType() {
  delete returnType;
  for (auto* argTy : argTypes) {
    delete argTy;
  }
}

ReturnType* FunctionType::get_return_type() { return returnType; }

ArgumentType* FunctionType::get_argument_type_at(u32 index) { return argTypes.at(index); }

Vec<ArgumentType*> FunctionType::get_argument_types() const { return argTypes; }

u64 FunctionType::get_argument_count() const { return argTypes.size(); }

String FunctionType::to_string() const {
  String result("(");
  for (usize i = 0; i < argTypes.size(); i++) {
    result += (argTypes.at(i)->to_string());
    if (i != (argTypes.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
  result += " -> " + returnType->to_string();
  return result;
}

} // namespace qat::ir