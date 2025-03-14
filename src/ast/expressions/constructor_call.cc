#include "./constructor_call.hpp"
#include "../../IR/logic.hpp"

#include <llvm/IR/Constants.h>

namespace qat::ast {

ir::Value* ConstructorCall::emit(EmitCtx* ctx) {
	SHOW("Constructor Call - Emitting type")
	if (not type && not is_type_inferred()) {
		ctx->Error("No type provided for the constructor call, and no type inferred from scope", fileRange);
	}
	auto* typ = type ? type.emit(ctx) : inferredType;
	if (type && is_type_inferred() && not typ->is_same(inferredType)) {
		ctx->Error("The type provided for the constructor call is " + ctx->color(typ->to_string()) +
		               ", which does not match the type inferred from scope, which is " +
		               ctx->color(inferredType->to_string()),
		           type ? type.get_range() : fileRange);
	}
	SHOW("Emitted type")
	if (typ->is_expanded()) {
		auto*                             eTy = typ->as_expanded();
		Vec<ir::Value*>                   valsIR;
		Vec<Pair<Maybe<bool>, ir::Type*>> valsType;

		auto hasDefaultConstructor = [=]() -> bool {
			if (typ->is_expanded() && typ->as_expanded()->has_default_constructor()) {
				return true;
			}
			auto& imps = typ->get_default_implementations();
			for (auto* done : imps) {
				if (done->has_default_constructor()) {
					return true;
				}
			}
			return false;
		};
		auto getDefaultConstructor = [=]() -> ir::Method* {
			if (typ->is_expanded() && typ->as_expanded()->has_default_constructor()) {
				return typ->as_expanded()->get_default_constructor();
			}
			auto& imps = typ->get_default_implementations();
			for (auto* done : imps) {
				if (done->has_default_constructor()) {
					return done->get_default_constructor();
				}
			}
			return nullptr;
		};

		auto hasFromConvertor = [=](Pair<Maybe<bool>, ir::Type*> const& typePair) -> bool {
			if (typ->is_expanded() && typ->as_expanded()->has_from_convertor(typePair.first, typePair.second)) {
				return true;
			}
			auto& imps = typ->get_default_implementations();
			for (auto* done : imps) {
				if (done->has_from_convertor(typePair.first, typePair.second)) {
					return true;
				}
			}
			return false;
		};
		auto getFromConvertor = [=](Pair<Maybe<bool>, ir::Type*> const& typePair) -> ir::Method* {
			if (typ->is_expanded() && typ->as_expanded()->has_from_convertor(typePair.first, typePair.second)) {
				return typ->as_expanded()->get_from_convertor(typePair.first, typePair.second);
			}
			auto& imps = typ->get_default_implementations();
			for (auto* done : imps) {
				if (done->has_from_convertor(typePair.first, typePair.second)) {
					return done->get_from_convertor(typePair.first, typePair.second);
				}
			}
			return nullptr;
		};

		auto hasConstructorWithTypes = [=](Vec<Pair<Maybe<bool>, ir::Type*>> const& types) -> bool {
			if (typ->is_expanded() && typ->as_expanded()->has_constructor_with_types(types)) {
				return true;
			}
			auto& imps = typ->get_default_implementations();
			for (auto* done : imps) {
				if (done->has_constructor_with_types(types)) {
					return true;
				}
			}
			return false;
		};
		auto getConstructorWithTypes = [=](Vec<Pair<Maybe<bool>, ir::Type*>> const& types) -> ir::Method* {
			if (typ->is_expanded() && typ->as_expanded()->has_constructor_with_types(types)) {
				return typ->as_expanded()->get_constructor_with_types(types);
			}
			auto& imps = typ->get_default_implementations();
			for (auto* done : imps) {
				if (done->has_constructor_with_types(types)) {
					return done->get_constructor_with_types(types);
				}
			}
			return nullptr;
		};

		for (auto* arg : args) {
			auto* argVal = arg->emit(ctx);
			valsType.push_back(
			    {argVal->is_ghost_ref() ? Maybe<bool>(argVal->is_variable()) : None, argVal->get_ir_type()});
			valsIR.push_back(argVal);
		}
		auto access = ctx->get_access_info();
		SHOW("Argument values emitted for function call")
		ir::Method* cons = nullptr;
		if (args.empty()) {
			if (not hasDefaultConstructor()) {
				ctx->Error(
				    "Type " + ctx->color(eTy->to_string()) +
				        " does not have a default constructor and hence argument values have to be provided in this constructor call",
				    fileRange);
			}
			cons = getDefaultConstructor();
			if (not cons->is_accessible(access)) {
				ctx->Error("The default constructor of type " + ctx->color(eTy->get_full_name()) +
				               " is not accessible here",
				           fileRange);
			}
			SHOW("Found default constructor")
		} else if (args.size() == 1) {
			if (not hasFromConvertor(valsType[0])) {
				ctx->Error("No from convertor found for type " + ctx->color(eTy->get_full_name()) + " with type " +
				               ctx->color(valsType.front().second->to_string()),
				           fileRange);
			}
			cons = getFromConvertor(valsType[0]);
			if (not cons->is_accessible(access)) {
				ctx->Error("This convertor of type " + ctx->color(eTy->get_full_name()) + " is not accessible here",
				           fileRange);
			}
			SHOW("Found convertor with type")
		} else {
			if (not hasConstructorWithTypes(valsType)) {
				ctx->Error("No matching constructor found for type " + ctx->color(eTy->get_full_name()), fileRange);
			}
			cons = getConstructorWithTypes(valsType);
			if (not cons->is_accessible(access)) {
				ctx->Error("This constructor of type " + ctx->color(eTy->get_full_name()) + " is not accessible here",
				           fileRange);
			}
			SHOW("Found constructor with types")
		}
		SHOW("Found convertor/constructor")
		auto argTys = cons->get_ir_type()->as_function()->get_argument_types();
		for (usize i = 1; i < argTys.size(); i++) {
			valsIR[i - 1] =
			    ir::Logic::handle_pass_semantics(ctx, argTys.at(i)->get_type(), valsIR[i - 1], args.at(i)->fileRange);
		}
		if (isLocalDecl()) {
			createIn = ctx->get_fn()->get_block()->new_local(irName->value, eTy, isVar, irName->range);
		}
		if (canCreateIn()) {
			if (not createIn->is_ref() && not createIn->is_ghost_ref()) {
				ctx->Error(
				    "Tried to optimise the constructor call by creating in-place, but the underlying type for in-place creation is " +
				        ctx->color(createIn->get_ir_type()->to_string()) + ", which is not a reference",
				    fileRange);
			}
			auto expTy =
			    createIn->is_ghost_ref() ? createIn->get_ir_type() : createIn->get_ir_type()->as_ref()->get_subtype();
			if (not type_check_create_in(eTy)) {
				ctx->Error("Tried to optimise the constructor call by creating in-place, but the inferred type is " +
				               ctx->color(eTy->to_string()) +
				               " which does not match with the underlying type for in-place creation which is " +
				               ctx->color(expTy->to_string()),
				           fileRange);
			}
		} else {
			createIn =
			    ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), eTy, isVar, fileRange);
		}
		Vec<llvm::Value*> valsLLVM;
		valsLLVM.push_back(createIn->get_llvm());
		for (auto* val : valsIR) {
			valsLLVM.push_back(val->get_llvm());
		}
		(void)cons->call(ctx->irCtx, valsLLVM, None, ctx->mod);
		return get_creation_result(ctx->irCtx, eTy);
	} else {
		ctx->Error("The provided type " + ctx->color(typ->to_string()) + " cannot be constructed",
		           type ? type.get_range() : fileRange);
	}
	return nullptr;
}

Json ConstructorCall::to_json() const {
	Vec<JsonValue> argsJson;
	for (auto* arg : args) {
		argsJson.push_back(arg->to_json());
	}
	return Json()
	    ._("nodeType", "constructorCall")
	    ._("hasType", (bool)type)
	    ._("type", (JsonValue)type)
	    ._("arguments", argsJson)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
