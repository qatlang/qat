#include "./operator_function.hpp"
#include "../show.hpp"
#include "expressions/operator.hpp"
#include "types/self_type.hpp"
#include <vector>

namespace qat::ast {

OperatorPrototype::~OperatorPrototype() {
	for (auto* arg : arguments) {
		std::destroy_at(arg);
	}
}

void OperatorPrototype::define(MethodState& state, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent);
	if (defineChecker) {
		auto defRes = defineChecker->emit(emitCtx);
		if (not defRes->get_ir_type()->is_bool()) {
			irCtx->Error("The define condition is expected to be of type " + irCtx->color("bool") +
			                 ", but got an expression of type " + irCtx->color(defRes->get_ir_type()->to_string()),
			             defineChecker->fileRange);
		}
		state.defineCondition = llvm::cast<llvm::ConstantInt>(defRes->get_llvm_constant())->getValue().getBoolValue();
		if (not state.defineCondition.value()) {
			return;
		}
	}
	if (metaInfo.has_value()) {
		state.metaInfo = metaInfo.value().toIR(emitCtx);
	}
	if (opr == Op::copyAssignment) {
		if (state.parent->is_done_skill() && state.parent->as_done_skill()->has_copy_assignment()) {
			irCtx->Error("Copy assignment operator already exists in this implementation " +
			                 irCtx->color(state.parent->as_done_skill()->to_string()),
			             fileRange);
		} else if (state.parent->is_expanded() && state.parent->as_expanded()->has_copy_assignment()) {
			irCtx->Error("Copy assignment operator already exists for the parent type " +
			                 irCtx->color(state.parent->as_expanded()->to_string()),
			             fileRange);
		}
		state.result = ir::Method::CopyAssignment(state.parent, nameRange,
		                                          state.metaInfo.has_value() && state.metaInfo->get_inline(),
		                                          argName.value(), fileRange, irCtx);
		return;
	} else if (opr == Op::moveAssignment) {
		if (state.parent->is_expanded() && state.parent->as_expanded()->has_move_assignment()) {
			irCtx->Error("Move assignment operator already exists for the parent type " +
			                 irCtx->color(state.parent->as_expanded()->get_full_name()),
			             fileRange);
		} else if (state.parent->is_done_skill() && state.parent->as_done_skill()->has_move_assignment()) {
			irCtx->Error("Move assignment operator already exists in this implementation " +
			                 irCtx->color(state.parent->as_done_skill()->to_string()),
			             fileRange);
		}
		state.result = ir::Method::MoveAssignment(state.parent, nameRange,
		                                          state.metaInfo.has_value() && state.metaInfo->get_inline(),
		                                          argName.value(), fileRange, irCtx);
		return;
	}
	if (opr == Op::subtract) {
		if (arguments.empty()) {
			opr = Op::minus;
		}
	}
	if (is_unary_operator(opr)) {
		if (!arguments.empty()) {
			irCtx->Error("Unary operators should have no arguments. Invalid definition "
			             "of unary operator " +
			                 irCtx->color(operator_to_string(opr)),
			             fileRange);
		}
	} else {
		if (arguments.size() != 1) {
			irCtx->Error("Invalid number of arguments for Binary operator " + irCtx->color(operator_to_string(opr)) +
			                 ". Binary operators should have only 1 argument",
			             fileRange);
		}
	}
	Vec<ir::Type*> generatedTypes;
	SHOW("Generating types")
	for (usize i = 0; i < arguments.size(); i++) {
		if (arguments[i]->is_member_arg()) {
			irCtx->Error("Member arguments are not allowed in operators", arguments[i]->get_name().range);
		} else if (arguments[i]->is_variadic_arg()) {
			irCtx->Error("Variadic argument is not allowed in operators", arguments[i]->get_name().range);
		} else {
			generatedTypes.push_back(arguments[i]->get_type()->emit(emitCtx));
		}
	}
	if (is_unary_operator(opr)) {
		if (state.parent->is_expanded() && state.parent->as_expanded()->has_unary_operator(operator_to_string(opr))) {
			irCtx->Error("Unary operator " + irCtx->color(operator_to_string(opr)) +
			                 " already exists for the parent type " +
			                 irCtx->color(state.parent->as_expanded()->get_full_name()),
			             fileRange);
		} else if (state.parent->is_done_skill() &&
		           state.parent->as_expanded()->has_unary_operator(operator_to_string(opr))) {
			irCtx->Error("Unary operator " + irCtx->color(operator_to_string(opr)) +
			                 " already exists in the implementation " +
			                 irCtx->color(state.parent->as_done_skill()->to_string()),
			             fileRange);
		}
	} else {
		if (state.parent->is_expanded() && (isVariationFn ? state.parent->as_expanded()->has_variation_binary_operator(
		                                                        operator_to_string(opr), {None, generatedTypes[0]})
		                                                  : state.parent->as_expanded()->has_normal_binary_operator(
		                                                        operator_to_string(opr), {None, generatedTypes[0]}))) {
			irCtx->Error(String(isVariationFn ? "Variation b" : "B") + "inary operator " +
			                 irCtx->color(operator_to_string(opr)) + " already exists for parent type " +
			                 irCtx->color(state.parent->as_expanded()->get_full_name()) +
			                 " with right hand side being type " + irCtx->color(generatedTypes.front()->to_string()),
			             fileRange);
		} else if (state.parent->is_done_skill() &&
		           (isVariationFn ? state.parent->as_done_skill()->has_variation_binary_operator(
		                                operator_to_string(opr), {None, generatedTypes[0]})
		                          : state.parent->as_done_skill()->has_normal_binary_operator(
		                                operator_to_string(opr), {None, generatedTypes[0]}))) {
			irCtx->Error(String(isVariationFn ? "Variation b" : "B") + "inary operator " +
			                 irCtx->color(operator_to_string(opr)) + " already exists in the implementation " +
			                 irCtx->color(state.parent->as_done_skill()->to_string()) +
			                 " with right hand side being type " + irCtx->color(generatedTypes.front()->to_string()),
			             fileRange);
		}
	}
	SHOW("Argument types generated")
	Vec<ir::Argument> args;
	SHOW("Setting variability of arguments")
	for (usize i = 0; i < generatedTypes.size(); i++) {
		if (arguments.at(i)->is_member_arg()) {
			args.push_back(ir::Argument::CreateMember(arguments.at(i)->get_name(), generatedTypes.at(i), i));
		} else {
			args.push_back(arguments.at(i)->is_variable()
			                   ? ir::Argument::CreateVariable(arguments.at(i)->get_name(),
			                                                  arguments.at(i)->get_type()->emit(emitCtx), i)
			                   : ir::Argument::Create(arguments.at(i)->get_name(), generatedTypes.at(i), i));
		}
	}
	SHOW("Variability setting complete")
	SHOW("About to create operator function")
	bool isSelfReturn = false;
	if (returnType->type_kind() == AstTypeKind::SELF_TYPE) {
		auto* selfRet = ((SelfType*)returnType);
		if (!selfRet->isJustType) {
			selfRet->isVarRef          = isVariationFn;
			selfRet->canBeSelfInstance = true;
			isSelfReturn               = true;
		}
	}
	auto retTy = returnType->emit(emitCtx);
	SHOW("Operator " + operator_to_string(opr) + " isVar: " << isVariationFn << " return type is "
	                                                        << retTy->to_string())
	state.result = ir::Method::CreateOperator(
	    state.parent, nameRange, !is_unary_operator(opr), isVariationFn, operator_to_string(opr),
	    state.metaInfo.has_value() && state.metaInfo->get_inline(), ir::ReturnType::get(retTy, isSelfReturn), args,
	    fileRange, emitCtx->get_visibility_info(visibSpec), irCtx);
	SHOW("Created IR operator")
}

Json OperatorPrototype::to_json() const {
	Vec<JsonValue> args;
	for (auto* arg : arguments) {
		auto aJson = Json()
		                 ._("name", arg->get_name())
		                 ._("type", arg->get_type() ? arg->get_type()->to_json() : Json())
		                 ._("is_member_argument", arg->is_member_arg())
		                 ._("is_variadic_argument", arg->is_variadic_arg());
		args.push_back(aJson);
	}
	return Json()
	    ._("nodeType", "operatorPrototype")
	    ._("isVariation", isVariationFn)
	    ._("operator", operator_to_string(opr))
	    ._("returnType", returnType->to_json())
	    ._("arguments", args)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

void OperatorDefinition::define(MethodState& state, ir::Ctx* irCtx) { prototype->define(state, irCtx); }

ir::Value* OperatorDefinition::emit(MethodState& state, ir::Ctx* irCtx) {
	auto* fnEmit = state.result;
	SHOW("Set active operator function: " << fnEmit->get_full_name())
	auto* block = new ir::Block(fnEmit, nullptr);
	SHOW("Created entry block")
	block->set_active(irCtx->builder);
	SHOW("Set new block as the active block")
	SHOW("About to allocate necessary arguments")
	auto  argIRTypes = fnEmit->get_ir_type()->as_function()->get_argument_types();
	auto* coreRefTy  = argIRTypes.at(0)->get_type()->as_reference();
	auto* self       = block->new_value("''", coreRefTy, prototype->isVariationFn,
	                                    coreRefTy->get_subtype()->as_struct()->get_name().range);
	irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(0u), self->get_llvm());
	self->load_ghost_reference(irCtx->builder);
	if ((prototype->opr == Op::copyAssignment) || (prototype->opr == Op::moveAssignment)) {
		auto* argVal = block->new_value(
		    prototype->argName->value,
		    ir::ReferenceType::get(prototype->opr == Op::moveAssignment, coreRefTy->get_subtype(), irCtx), false,
		    prototype->argName->range);
		irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(1u), argVal->get_llvm());
	} else {
		for (usize i = 1; i < argIRTypes.size(); i++) {
			SHOW("Argument type is " << argIRTypes.at(i)->get_type()->to_string())
			auto* argVal = block->new_value(argIRTypes.at(i)->get_name(), argIRTypes.at(i)->get_type(), true,
			                                prototype->arguments.at(i - 1)->get_name().range);
			SHOW("Created local value for the argument")
			irCtx->builder.CreateStore(fnEmit->get_llvm_function()->getArg(i), argVal->get_alloca(), false);
		}
		SHOW("Operator Return type is " << fnEmit->get_ir_type()->as_function()->get_return_type()->to_string())
	}
	emit_sentences(
	    sentences,
	    EmitCtx::get(irCtx, state.parent->get_module())->with_member_parent(state.parent)->with_function(fnEmit));
	ir::function_return_handler(irCtx, fnEmit, sentences.empty() ? fileRange : sentences.back()->fileRange);
	SHOW("Sentences emitted")
	return nullptr;
}

Json OperatorDefinition::to_json() const {
	Vec<JsonValue> sntcs;
	for (auto* sentence : sentences) {
		sntcs.push_back(sentence->to_json());
	}
	return Json()
	    ._("nodeType", "operatorDefinition")
	    ._("prototype", prototype->to_json())
	    ._("body", sntcs)
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
