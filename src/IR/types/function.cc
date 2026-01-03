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

FunctionType::FunctionType(ReturnType* _retType, Vec<ArgumentType*> _argTypes, Maybe<Variadics> _variadics,
                           llvm::LLVMContext&)
    : returnType(_retType), argTypes(std::move(_argTypes)), variadics(_variadics) {
	SHOW("Creating function type")
	linkingName = "qat'fn_type:[(";
	for (usize i = 0; i < argTypes.size(); i++) {
		linkingName += argTypes.at(i)->get_type()->get_name_for_linking();
		if ((i != (argTypes.size() - 1)) || variadics.has_value()) {
			linkingName += ", ";
		}
	}
	if (variadics.has_value()) {
		switch (variadics.value().kind) {
			case VariadicsKind::NORMAL: {
				linkingName += "variadic.normal";
				break;
			}
			case VariadicsKind::LEGACY: {
				linkingName += "variadic.legacy";
				break;
			}
			case VariadicsKind::TYPED: {
				linkingName += "variadic.typed." + variadics.value().type->get_name_for_linking();
				break;
			}
		}
	}
	linkingName += ") -> " + returnType->get_type()->get_name_for_linking() + "]";
	Vec<llvm::Type*> argTys;
	for (usize i = 0; i < argTypes.size(); i++) {
		SHOW("Arg name is " << argTypes[i]->get_name())
		argTys.push_back(argTypes[i]->get_type()->get_llvm_type());
	}
	SHOW("Got arg llvm types in FunctionType")
	llvmType = llvm::FunctionType::get(returnType->get_type()->get_llvm_type(), argTys, variadics.has_value());
}

FunctionType::~FunctionType() {
	std::destroy_at(returnType);
	for (auto* argTy : argTypes) {
		std::destroy_at(argTy);
	}
}

String FunctionType::to_string() const {
	String result("(");
	for (usize i = 0; i < argTypes.size(); i++) {
		result += (argTypes.at(i)->to_string());
		if ((i != (argTypes.size() - 1)) || variadics.has_value()) {
			result += ", ";
		}
	}
	if (variadics.has_value()) {
		result += variadics.value().to_string();
	}
	result += ")";
	result += " -> " + returnType->to_string();
	return result;
}

} // namespace qat::ir
