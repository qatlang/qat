#include "./math.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/native_type.hpp"
#include "../../IR/types/vector.hpp"

#include <llvm/IR/Intrinsics.h>

namespace qat::ast {

const std::set<String> MetaMath::functionNames = {"abs",   "max",  "min",  "powi",  "sqrt",  "sin",  "cos",   "tan",
                                                  "asin",  "acos", "atan", "sinh",  "cosh",  "tanh", "exp",   "exp2",
                                                  "exp10", "log",  "log2", "log10", "floor", "ceil", "trunc", "round"};

ir::Value* MetaMath::emit(EmitCtx* ctx) {
	auto oneArgCheck = [&]() {
		if (arguments.size() != 1) {
			ctx->Error("The mathematical function " + ctx->color(name.value) + " expects exactly one argument",
			           fileRange);
		}
	};
	if (name.value == "abs") {
		oneArgCheck();
		auto arg           = arguments[0]->emit(ctx);
		auto intrinsicType = llvm::Intrinsic::abs;
		if (arg->get_pass_type()->is_underlying_type_integer() ||
		    (arg->get_pass_type()->is_vector() && arg->get_pass_type()->as_vector()->is_fixed() &&
		     arg->get_pass_type()->as_vector()->get_element_type()->is_underlying_type_integer())) {
			intrinsicType = llvm::Intrinsic::abs;
		} else if (arg->get_pass_type()->is_underlying_type_float() ||
		           (arg->get_pass_type()->is_vector() && arg->get_pass_type()->as_vector()->is_fixed() &&
		            arg->get_pass_type()->as_vector()->get_element_type()->is_underlying_type_float())) {
			intrinsicType = llvm::Intrinsic::fabs;
		} else {
			ctx->Error("The mathematical function " + ctx->color(name.value) +
			               " expects an argument of either a signed integer, a fixed vector of signed integers,"
			               " a floating point number, or a fixed vector of floating point numbers",
			           fileRange);
		}
		arg     = ir::Logic::handle_pass_semantics(ctx, arg->get_pass_type(), arg, arguments[0]->fileRange);
		auto fn = llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), intrinsicType,
		                                                  {arg->get_ir_type()->get_llvm_type()});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(
		        fn->getFunctionType(), fn,
		        {arg->get_llvm(), llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)}),
		    arg->get_ir_type(), true);
	} else if (name.value == "max" || name.value == "min") {
		if (arguments.size() != 2) {
			ctx->Error("The mathematical function " + ctx->color(name.value) + " expects 2 arguments", fileRange);
		}
		auto oneVal = arguments[0]->emit(ctx);
		auto twoVal = arguments[1]->emit(ctx);
		oneVal      = ir::Logic::handle_pass_semantics(ctx, oneVal->get_pass_type(), oneVal, arguments[0]->fileRange);
		twoVal      = ir::Logic::handle_pass_semantics(ctx, twoVal->get_pass_type(), twoVal, arguments[1]->fileRange);
		auto intrinsicType = llvm::Intrinsic::smax;
		if (not oneVal->get_ir_type()->is_same(twoVal->get_ir_type())) {
			ctx->Error("The mathematical function " + ctx->color(name.value) +
			               " expects two arguments of the same type. The first argument is of type " +
			               ctx->color(oneVal->get_ir_type()->to_string()) + ", but the second argument is of type " +
			               ctx->color(twoVal->get_ir_type()->to_string()),
			           fileRange);
		}
		if (oneVal->get_ir_type()->is_underlying_type_integer() ||
		    (oneVal->get_ir_type()->is_vector() && oneVal->get_ir_type()->as_vector()->is_fixed() &&
		     oneVal->get_ir_type()->as_vector()->get_element_type()->is_underlying_type_integer())) {
			intrinsicType = name.value == "max" ? llvm::Intrinsic::smax : llvm::Intrinsic::smin;
		} else if (oneVal->get_ir_type()->is_underlying_type_unsigned() ||
		           (oneVal->get_ir_type()->is_vector() && oneVal->get_ir_type()->as_vector()->is_fixed() &&
		            oneVal->get_ir_type()->as_vector()->get_element_type()->is_underlying_type_unsigned())) {
			intrinsicType = name.value == "max" ? llvm::Intrinsic::umax : llvm::Intrinsic::umin;
		} else if (oneVal->get_ir_type()->is_underlying_type_float() ||
		           (oneVal->get_ir_type()->is_vector() && oneVal->get_ir_type()->as_vector()->is_fixed() &&
		            oneVal->get_ir_type()->as_vector()->get_element_type()->is_underlying_type_float())) {
			intrinsicType = name.value == "max" ? llvm::Intrinsic::maximum : llvm::Intrinsic::minimum;
		} else {
			ctx->Error(
			    "The mathematical function " + ctx->color(name.value) +
			        " expects two arguments of the same type. The type can either be a signed integer,"
			        " a fixed vector of signed integers, an unsigned integer, a fixed vector of unsigned integers,"
			        " a floating point number or a fixed vector of floating point numbers",
			    fileRange);
		}
		auto fn = llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), intrinsicType,
		                                                  {oneVal->get_ir_type()->get_llvm_type()});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(fn->getFunctionType(), fn, {oneVal->get_llvm(), twoVal->get_llvm()}),
		    oneVal->get_ir_type(), true);
	} else if (name.value == "powi") {
		if (arguments.size() != 2) {
			ctx->Error("The mathematical function " + ctx->color("powi") +
			               " expects two arguments. The first argument can be a value of"
			               " any float datatype, and the second argument should be of type " +
			               ctx->color("int"),
			           fileRange);
		}
		auto oneVal = arguments[0]->emit(ctx);
		auto intTy  = ir::NativeType::get_int(ctx->irCtx);
		if (arguments[1]->has_type_inferrance()) {
			arguments[1]->as_type_inferrable()->set_inference_type(intTy);
		}
		auto twoVal = arguments[1]->emit(ctx);
		if ((not oneVal->get_pass_type()->is_float()) &&
		    not(oneVal->get_pass_type()->is_native_type() &&
		        oneVal->get_pass_type()->as_native_type()->get_subtype()->is_float())) {
			ctx->Error("The first argument for this mathematical function is expected"
			           " to be of a float datatype, but got an expression of type " +
			               ctx->color(oneVal->get_pass_type()->to_string()),
			           arguments[0]->fileRange);
		}
		if (not twoVal->get_pass_type()->is_same(intTy)) {
			ctx->Error("The second argument for this mathematical function is expected to be of " +
			               ctx->color(intTy->to_string()) + " datatype, but got an expression of type " +
			               ctx->color(twoVal->get_pass_type()->to_string()),
			           arguments[1]->fileRange);
		}
		oneVal  = ir::Logic::handle_pass_semantics(ctx, oneVal->get_pass_type(), oneVal, arguments[0]->fileRange);
		twoVal  = ir::Logic::handle_pass_semantics(ctx, twoVal->get_pass_type(), twoVal, arguments[1]->fileRange);
		auto fn = llvm::Intrinsic::getOrInsertDeclaration(
		    ctx->mod->get_llvm_module(), llvm::Intrinsic::powi,
		    {oneVal->get_ir_type()->get_llvm_type(), twoVal->get_ir_type()->get_llvm_type()});
		return ir::Value::get(
		    ctx->irCtx->builder.CreateCall(fn->getFunctionType(), fn, {oneVal->get_llvm(), twoVal->get_llvm()}),
		    oneVal->get_ir_type(), true);
	} else if (name.value == "sqrt" || name.value == "sin" || name.value == "cos" || name.value == "tan" ||
	           name.value == "asin" || name.value == "acos" || name.value == "atan" || name.value == "sinh" ||
	           name.value == "cosh" || name.value == "tanh" || name.value == "exp" || name.value == "exp2" ||
	           name.value == "exp10" || name.value == "log" || name.value == "log2" || name.value == "log10" ||
	           name.value == "floor" || name.value == "ceil" || name.value == "trunc" || name.value == "round") {
		oneArgCheck();
		llvm::Intrinsic::IndependentIntrinsics intrinsicType;
		if (name.value == "sqrt") {
			intrinsicType = llvm::Intrinsic::sqrt;
		} else if (name.value == "sin") {
			intrinsicType = llvm::Intrinsic::sin;
		} else if (name.value == "cos") {
			intrinsicType = llvm::Intrinsic::cos;
		} else if (name.value == "tan") {
			intrinsicType = llvm::Intrinsic::tan;
		} else if (name.value == "asin") {
			intrinsicType = llvm::Intrinsic::asin;
		} else if (name.value == "acos") {
			intrinsicType = llvm::Intrinsic::acos;
		} else if (name.value == "atan") {
			intrinsicType = llvm::Intrinsic::atan;
		} else if (name.value == "sinh") {
			intrinsicType = llvm::Intrinsic::sinh;
		} else if (name.value == "cosh") {
			intrinsicType = llvm::Intrinsic::cosh;
		} else if (name.value == "tanh") {
			intrinsicType = llvm::Intrinsic::tanh;
		} else if (name.value == "exp") {
			intrinsicType = llvm::Intrinsic::exp;
		} else if (name.value == "exp2") {
			intrinsicType = llvm::Intrinsic::exp2;
		} else if (name.value == "exp10") {
			intrinsicType = llvm::Intrinsic::exp10;
		} else if (name.value == "log") {
			intrinsicType = llvm::Intrinsic::log;
		} else if (name.value == "log2") {
			intrinsicType = llvm::Intrinsic::log2;
		} else if (name.value == "log10") {
			intrinsicType = llvm::Intrinsic::log10;
		} else if (name.value == "floor") {
			intrinsicType = llvm::Intrinsic::floor;
		} else if (name.value == "ceil") {
			intrinsicType = llvm::Intrinsic::ceil;
		} else if (name.value == "trunc") {
			intrinsicType = llvm::Intrinsic::trunc;
		} else if (name.value == "round") {
			intrinsicType = llvm::Intrinsic::round;
		}
		auto val = arguments[0]->emit(ctx);
		if (not val->get_pass_type()->is_underlying_type_float() &&
		    not(val->get_pass_type()->is_vector() && val->get_pass_type()->as_vector()->is_fixed() &&
		        val->get_pass_type()->as_vector()->get_element_type()->is_underlying_type_float())) {
			ctx->Error(
			    "The argument of the mathematical function " + ctx->color(name.value) +
			        " is expected to be of either float type or a fixed vector of floats, but got an expression of type " +
			        ctx->color(val->get_pass_type()->to_string()) + " instead",
			    arguments[0]->fileRange);
		}
		val     = ir::Logic::handle_pass_semantics(ctx, val->get_pass_type(), val, arguments[0]->fileRange);
		auto fn = llvm::Intrinsic::getOrInsertDeclaration(ctx->mod->get_llvm_module(), intrinsicType,
		                                                  {val->get_ir_type()->get_llvm_type()});
		return ir::Value::get(ctx->irCtx->builder.CreateCall(fn->getFunctionType(), fn, {val->get_llvm()}),
		                      val->get_ir_type(), true);
	}
}

} // namespace qat::ast
