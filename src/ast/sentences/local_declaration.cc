#include "./local_declaration.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/pointer.hpp"
#include "../../show.hpp"

#include <llvm/IR/Instructions.h>

namespace qat::ast {

ir::Value* LocalDeclaration::emit(EmitCtx* ctx) {
	auto* block = ctx->get_fn()->get_block();
	if (block->has_value(name.value)) {
		ctx->Error("A " + ctx->color("let") + "binding named " + ctx->color(name.value) +
		               " already exists in this scope. Please change name of this " + ctx->color("let") +
		               " binding or check the logic",
		           fileRange,
		           std::make_pair("The previous " + ctx->color("let") + " binding was found here",
		                          block->get_value(name.value)->get_file_range()));
	} else if (block->has_used_value(name.value)) {
		ctx->Error("A " + ctx->color("use") + " binding named " + ctx->color(name.value) +
		               " already exists in this scope. Please change name of this " + ctx->color("let") +
		               " binding or check the logic",
		           fileRange,
		           std::make_pair("The previous " + ctx->color("use") + " binding was found here",
		                          block->get_used_value(name.value)->get_range()));
	} else {
		ctx->genericNameCheck(name.value, name.range);
	}
	ir::Type* declType = nullptr;

	SHOW("Type for local declaration is " << (type ? type->to_string() : "not provided"));

	auto typeCheck = [&]() {
		if (declType) {
			if (declType->is_maybe() && not variability) {
				ctx->irCtx->Warning(
				    "The type of the declaration is " + ctx->irCtx->highlightWarning(declType->to_string()) +
				        ", but the local declaration is not a variable. And hence, it might not be usable",
				    fileRange);
			}
			if (not declType->is_type_sized()) {
				ctx->Error("The type " + ctx->color(declType->to_string()) +
				               " is not sized and hence cannot be allocated",
				           fileRange);
			}
			if (declType->is_ptr() && declType->as_ptr()->get_owner().is_prerun()) {
				ctx->Error("Prerun " + String(declType->as_ptr()->is_multi() ? "multi-pointers" : "pointers") +
				               " cannot be used in " + ctx->color("let") + " bindings in normal functions",
				           fileRange);
			}
		}
	};

	ir::Value* expVal = nullptr;
	if (value.has_value()) {
		SHOW("LocalDecl value kind is " << (int)value.value()->nodeType())
		if (type) {
			declType = type->emit(ctx);
			typeCheck();
		}
		if (declType && value.value()->has_type_inferrance()) {
			value.value()->as_type_inferrable()->set_inference_type(declType);
		}
		if (value.value()->isInPlaceCreatable() && declType) {
			SHOW("LocalDecl value is in-place creatable")
			value.value()->asInPlaceCreatable()->setCreateIn(
			    ctx->get_fn()->get_block()->new_local(name.value, declType, variability, ctx->irCtx, name.range));
			return value.value()->emit(ctx);
		} else if (value.value()->isLocalDeclCompatible()) {
			SHOW("LocalDecl value is compatible")
			auto* localDeclCompat   = value.value()->asLocalDeclCompatible();
			localDeclCompat->irName = name;
			localDeclCompat->isVar  = variability;
			SHOW("Emitting value")
			auto valRes = value.value()->emit(ctx);
			if (declType && not valRes->get_ir_type()->is_same(declType)) {
				ctx->Error("The type of this local declaration is " + ctx->color(declType->to_string()) +
				               " but the expression is of type " + ctx->color(valRes->get_ir_type()->to_string()),
				           value.value()->fileRange);
			}
			return valRes;
		} else {
			SHOW("Emitting value")
			expVal = value.value()->emit(ctx);
			SHOW("Pass type of value to be assigned to local value " << name.value << " is "
			                                                         << expVal->get_pass_type()->to_string())
			if (not declType) {
				declType = expVal->get_pass_type();
				typeCheck();
			}
			expVal   = ir::Logic::handle_pass_semantics(ctx, declType, expVal, value.value()->fileRange);
			auto res = ctx->get_fn()->get_block()->new_local(name.value, declType, variability, ctx->irCtx, name.range);
			ctx->irCtx->builder.CreateStore(expVal->get_llvm(), res->get_llvm(), false);
			return nullptr;
		}
	} else {
		if (isUninitialised) {
			if (not type) {
				ctx->Error("Ignoring the initialisation of the " + ctx->color("let") +
				               " binding using _ is only allowed if a type is provided. Use the syntax " +
				               ctx->color("let " + String(variability ? "var " : "") + name.value + " :: Type = _."),
				           fileRange);
			}
			declType = type->emit(ctx);
			typeCheck();
			if (not declType->has_simple_copy() && not declType->has_simple_move()) {
				ctx->Error("Ignoring the initialisation of the " + ctx->color("let") +
				               " binding using _ is only allowed if the type has both"
				               " simple-copy and simple-move. The type provided is " +
				               ctx->color(declType->to_string()) + " which does not satisfy this condition",
				           fileRange);
			}
			(void)ctx->get_fn()->get_block()->new_local(name.value, declType, variability, ctx->irCtx, name.range);
			return nullptr;
		} else if (type) {
			declType = type->emit(ctx);
			typeCheck();
			if (not declType->is_maybe()) {
				ctx->Error(
				    "The type of the " + ctx->color("let") + " binding is " + ctx->color(declType->to_string()) +
				        ". Value for initialisation can only be skipped if it is a " + ctx->color("maybe") + " type." +
				        (declType->has_simple_move()
				             ? ((declType->is_underlying_type_integer() || declType->is_underlying_type_unsigned() ||
				                 declType->is_underlying_type_float())
				                    ? ("\n- Since the type " + ctx->color(declType->to_string()) +
				                       " is a numeric type, it is recommended to use 0 for initialisation.")
				                    : ("\n- Since the type " + ctx->color(declType->to_string()) +
				                       " has simple-move, you can use the expression" + ctx->color("zero") +
				                       " to zero-initialise the data."))
				             : "") +
				        ((declType->has_simple_copy() && declType->has_simple_move())
				             ? ("\n- Since the type " + ctx->color(declType->to_string()) +
				                " has both simple-copy and simple-move, you may ignore the initialisation of the " +
				                ctx->color("let") +
				                " binding using the _ expression. Do this only if you must, and if know what you"
				                " are doing. This means that the binding may have a random value in"
				                " it (don't use this for randomness, please). Use the syntax " +
				                ctx->color("let " + String(variability ? "var " : "") + name.value +
				                           " :: " + type->to_string() + " = _.") +
				                " to ignore initialisation.")
				             : ""),
				    fileRange);
			}
			auto result =
			    ctx->get_fn()->get_block()->new_local(name.value, declType, variability, ctx->irCtx, name.range);
			ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(declType->get_llvm_type()),
			                                result->get_llvm());
			return nullptr;
		} else {
			ctx->Error("This " + ctx->color("let") +
			               " binding does not have a type associated with it and also"
			               " does not have a value provided for initialisation."
			               " If a value is provided, the type can be inferred from it."
			               " If a type is provided and if it is a " +
			               ctx->color("maybe") + " type, the " + ctx->color("let") + " binding can be initialised to " +
			               ctx->color("none") + " in the absence of a value.",
			           fileRange);
			std::unreachable();
		}
	}
}

} // namespace qat::ast
