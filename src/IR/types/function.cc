#include "./function.hpp"
#include "../../show.hpp"
#include "./reference.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

ReturnType::ReturnType(Type* _retTy, bool _isRetSelfRef) : retTy(_retTy), isReturnSelfRef(_isRetSelfRef) {}

Type* ReturnType::get_type() const { return retTy; }

bool ReturnType::is_return_self() const { return isReturnSelfRef; }

String ReturnType::to_string() const { return isReturnSelfRef ? "''" : retTy->to_string(); }

FunctionType::FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, llvm::LLVMContext& llctx)
	: returnType(_retType), argTypes(std::move(_argTypes)),
	  isVariadicArgs((not argTypes.empty()) && (argTypes.back()->is_variadic_argument())) {
	SHOW("Creating function type")
	linkingName = "qat'fn_type:[(";
	for (usize i = 0; i < argTypes.size(); i++) {
		linkingName += argTypes.at(i)->get_type()->get_name_for_linking();
		if ((i != (argTypes.size() - 1)) || isVariadicArgs) {
			linkingName += ", ";
		}
	}
	if (isVariadicArgs) {
		linkingName += "variadic";
	}
	linkingName += ") -> " + returnType->get_type()->get_name_for_linking() + "]";
	Vec<llvm::Type*> argTys;
	for (usize i = 0; i < (argTypes.size() - (isVariadicArgs ? 1 : 0)); i++) {
		SHOW("Arg name is " << argTypes[i]->get_name())
		argTys.push_back(argTypes[i]->get_type()->get_llvm_type());
	}
	SHOW("Got arg llvm types in FunctionType")
	llvmType = llvm::FunctionType::get(returnType->get_type()->get_llvm_type(), argTys, isVariadicArgs);
}

FunctionType::~FunctionType() {
	std::destroy_at(returnType);
	for (auto* argTy : argTypes) {
		std::destroy_at(argTy);
	}
}

ReturnType* FunctionType::get_return_type() { return returnType; }

ArgumentType* FunctionType::get_argument_type_at(u64 index) {
	return (isVariadicArgs && (index == (argTypes.size() - 1))) ? nullptr : argTypes.at(index);
}

Vec<ArgumentType*> FunctionType::get_argument_types() const { return argTypes; }

u64 FunctionType::get_argument_count() const { return argTypes.size() - (isVariadicArgs ? 1 : 0); }

String FunctionType::to_string() const {
	String result("(");
	for (usize i = 0; i < argTypes.size(); i++) {
		result += (argTypes.at(i)->to_string());
		if ((i != (argTypes.size() - 1)) || isVariadicArgs) {
			result += ", ";
		}
	}
	if (isVariadicArgs) {
		result += "variadic";
	}
	result += ")";
	result += " -> " + returnType->to_string();
	return result;
}

} // namespace qat::ir
