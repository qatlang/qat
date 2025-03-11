#include "./method_call.hpp"
#include "../../IR/types/vector.hpp"

namespace qat::ast {

ir::PrerunValue* PrerunMemberFnCall::emit(EmitCtx* ctx) {
	SHOW("Prerun member function call " << memberName.value)
	auto inst = instance->emit(ctx);
	SHOW("Instance emitted")
	if (inst->get_ir_type()->is_typed()) {
		return handle_type_wrap_functions(inst, arguments, memberName, ctx, fileRange);
	} else {
		ctx->Error("Expression is of type " + ctx->color(inst->get_ir_type()->to_string()) +
		               " and hence doesn't support prerun member function call",
		           fileRange);
	}
	return nullptr;
}

ir::PrerunValue* handle_type_wrap_functions(ir::PrerunValue* typed, Vec<PrerunExpression*> const& arguments,
                                            Identifier memberName, EmitCtx* ctx, FileRange fileRange) {
	Vec<Expression*> args;
	for (auto arg : arguments) {
		args.push_back(arg);
	}
	return handle_type_wrap_functions(typed, args, memberName, ctx, fileRange);
}

ir::PrerunValue* handle_type_wrap_functions(ir::PrerunValue* typed, Vec<Expression*> const& arguments,
                                            Identifier memberName, EmitCtx* ctx, FileRange fileRange) {
	auto zeroArgCheck = [&]() {
		if (arguments.size() != 0) {
			ctx->Error("The type trait " + ctx->color(memberName.value) +
			               " of wrapped type expects zero arguments, but " +
			               ctx->color(std::to_string(arguments.size())) + " were provided",
			           fileRange);
		}
	};
	auto subTy = ir::TypeInfo::get_for(typed->get_llvm_constant())->type;
	if (memberName.value == "byte_size") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(
		        llvm::Type::getInt64Ty(ctx->irCtx->llctx),
		        subTy->is_type_sized()
		            ? ctx->mod->get_llvm_module()->getDataLayout().getTypeStoreSize(subTy->get_llvm_type())
		            : 1u),
		    ir::UnsignedType::create(64u, ctx->irCtx));
	} else if (memberName.value == "bit_size") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(
		        llvm::Type::getInt64Ty(ctx->irCtx->llctx),
		        subTy->is_type_sized()
		            ? ctx->mod->get_llvm_module()->getDataLayout().getTypeStoreSizeInBits(subTy->get_llvm_type())
		            : 8u),
		    ir::UnsignedType::create(64u, ctx->irCtx));
	} else if (memberName.value == "name") {
		zeroArgCheck();
		auto result = subTy->to_string();
		return ir::PrerunValue::get(
		    llvm::ConstantStruct::get(
		        llvm::cast<llvm::StructType>(ir::TextType::get(ctx->irCtx)->get_llvm_type()),
		        {ctx->irCtx->builder.CreateGlobalStringPtr(result, ctx->irCtx->get_global_string_name(), 0U,
		                                                   ctx->mod->get_llvm_module()),
		         llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), result.length())}),
		    ir::TextType::get(ctx->irCtx));
	} else if (memberName.value == "is_packed") {
		zeroArgCheck();
		if (not subTy->get_llvm_type()->isStructTy()) {
			ctx->Error("The wrapped type is not a struct and hence the type trait " + ctx->color(memberName.value) +
			               " cannot be used. Make sure to check the " + ctx->color("is_underlying_struct_type") +
			               " trait is satisfied first",
			           fileRange);
		}
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                           subTy->get_llvm_type()->isStructTy() &&
		                                   llvm::cast<llvm::StructType>(subTy->get_llvm_type())->isPacked()
		                               ? 1u
		                               : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_any_unsigned_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                           (subTy->is_unsigned() ||
		                            (subTy->is_native_type() && subTy->as_native_type()->get_subtype()->is_unsigned()))
		                               ? 1u
		                               : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_any_signed_type") {
		zeroArgCheck();
		auto subTy = ir::TypeInfo::get_for(typed->get_llvm_constant())->type;
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                           (subTy->is_integer() ||
		                            (subTy->is_native_type() && subTy->as_native_type()->get_subtype()->is_integer()))
		                               ? 1u
		                               : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_any_float_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(
		        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		        (subTy->is_float() || (subTy->is_native_type() && subTy->as_native_type()->get_subtype()->is_float()))
		            ? 1u
		            : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_unsigned_int_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_unsigned() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_signed_int_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_integer() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_float_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_float() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_tuple_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_tuple() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_text_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_text() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_struct") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_struct() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_vector_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_vector() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_underlying_struct_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                                                   subTy->get_llvm_type()->isStructTy() ? 1u : 0u),
		                            ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_mix_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_mix() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_array_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_array() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_choice_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_choice() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_flag_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_flag() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_maybe_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_maybe() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_result_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_result() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_void_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_void() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_future_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_future() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_mark_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_mark() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_slice_type") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                           (subTy->is_mark() && subTy->as_mark()->is_slice()) ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "has_simple_copy") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->has_simple_copy() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "has_simple_move") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->has_simple_move() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_copy_constructible") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_copy_constructible() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_copy_assignable") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_copy_assignable() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_move_constructible") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_move_constructible() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_move_assignable") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->is_move_assignable() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "has_copy") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(
		        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		        (subTy->has_simple_copy()) || (subTy->is_copy_constructible() && subTy->is_copy_assignable()) ? 1u
		                                                                                                      : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "has_move") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(
		        llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		        (subTy->has_simple_move()) || (subTy->is_move_constructible() && subTy->is_move_assignable()) ? 1u
		                                                                                                      : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "is_default_constructible") {
		zeroArgCheck();
		return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                                                   subTy->is_default_constructible() ? 1u : 0u),
		                            ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "has_prerun_default_value") {
		zeroArgCheck();
		return ir::PrerunValue::get(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
		                                                   subTy->has_prerun_default_value() ? 1u : 0u),
		                            ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "can_be_prerun") {
		zeroArgCheck();
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), subTy->can_be_prerun() ? 1u : 0u),
		    ir::UnsignedType::create_bool(ctx->irCtx));
	} else if (memberName.value == "get_element_count") {
		zeroArgCheck();
		if (subTy->is_array()) {
			return ir::PrerunValue::get(
			    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), subTy->as_array()->get_length()),
			    ir::UnsignedType::create(64u, ctx->irCtx));
		} else if (subTy->is_tuple()) {
			return ir::PrerunValue::get(
			    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), subTy->as_tuple()->getSubTypeCount()),
			    ir::UnsignedType::create(64u, ctx->irCtx));
		} else if (subTy->is_vector() && subTy->as_vector()->is_fixed()) {
			return ir::PrerunValue::get(
			    llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), subTy->as_vector()->get_count()),
			    ir::UnsignedType::create(64u, ctx->irCtx));
		} else {
			ctx->Error(
			    "This attribute can only be used for array, fixed vector or tuple types, make sure that the type is appropriate before querying this attribute",
			    fileRange);
		}
	} else {
		ctx->Error(ctx->color(memberName.value) + " is not a recognised attribute for the wrapped type",
		           memberName.range);
	}
	return nullptr;
}

String PrerunMemberFnCall::to_string() const {
	String argStr;
	for (usize i = 0; i < arguments.size(); i++) {
		argStr += arguments[i]->to_string();
		if (i != (arguments.size() - 1)) {
			argStr += ", ";
		}
	}
	return instance->to_string() + "'" + memberName.value + "(" + argStr + ")";
}

Json PrerunMemberFnCall::to_json() const {
	Vec<JsonValue> argJson;
	for (auto arg : arguments) {
		argJson.push_back(arg->to_json());
	}
	return Json()
	    ._("instance", instance->to_json())
	    ._("memberName", memberName)
	    ._("arguments", argJson)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
