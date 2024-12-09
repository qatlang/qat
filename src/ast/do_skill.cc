#include "./do_skill.hpp"
#include "../IR/skill.hpp"
#include "./destructor.hpp"
#include "./method.hpp"
#include "./operator_function.hpp"
#include "constructor.hpp"
#include "convertor.hpp"

namespace qat::ast {

void DoSkill::create_entity(ir::Mod* parent, ir::Ctx* irCtx) {
	if (isDefaultSkill) {
		entityState = parent->add_entity(None, ir::EntityType::defaultDoneSkill, this, ir::EmitPhase::phase_3);
		entityState->phaseToPartial = ir::EmitPhase::phase_2;
		for (auto memFn : memberDefinitions) {
			memFn->prototype->add_to_parent(entityState, irCtx);
		}
	}
}
void DoSkill::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
	if (isDefaultSkill) {
		auto ctx = EmitCtx::get(irCtx, parent);
		targetType->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
		if (defaultConstructor) {
			defaultConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
			                                                   entityState, ctx);
			defaultConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		if (copyConstructor) {
			copyConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
			                                                entityState, ctx);
			copyConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		if (moveConstructor) {
			moveConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
			                                                entityState, ctx);
			moveConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		if (copyAssignment) {
			copyAssignment->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
			                                               entityState, ctx);
			copyAssignment->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		if (moveAssignment) {
			moveAssignment->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
			                                               entityState, ctx);
			moveAssignment->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		if (destructorDefinition) {
			destructorDefinition->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState,
			                                          ctx);
		}
		for (auto cons : constructorDefinitions) {
			cons->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
			cons->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		for (auto conv : convertorDefinitions) {
			conv->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
			conv->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		for (auto memFn : memberDefinitions) {
			memFn->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
			memFn->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
		for (auto opr : operatorDefinitions) {
			opr->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
			opr->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
		}
	}
}
void DoSkill::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) {
	if (phase == ir::EmitPhase::phase_1) {
		define_done_skill(parent, irCtx);
	} else if (phase == ir::EmitPhase::phase_2) {
		define_members(irCtx);
	} else if (phase == ir::EmitPhase::phase_3) {
		emit_members(irCtx);
	}
}

void DoSkill::define_done_skill(ir::Mod* mod, ir::Ctx* irCtx) {
	auto* target = targetType->emit(EmitCtx::get(irCtx, mod));
	if (target->is_region() || target->is_reference() || target->is_typed() || target->is_function() ||
	    target->is_void()) {
		irCtx->Error("Creating a default implementation for " + irCtx->color(target->to_string()) + " is not allowed",
		             fileRange);
	}
	if (isDefaultSkill) {
		doneSkill = ir::DoneSkill::create_extension(mod, fileRange, target, targetType->fileRange);
		if (has_copy_constructor()) {
			irCtx->Error(
			    "Copy constructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
			    copyConstructor->fileRange);
		}
		if (has_copy_assignment()) {
			irCtx->Error(
			    "Copy assignment operator is not allowed in default implementations, but only in the original type, if the type is a struct type",
			    copyAssignment->fileRange);
		}
		if (has_move_constructor()) {
			irCtx->Error(
			    "Move constructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
			    moveConstructor->fileRange);
		}
		if (has_move_assignment()) {
			irCtx->Error(
			    "Move assignment operator is not allowed in default implementations, but only in the original type, if the type is a struct type",
			    moveAssignment->fileRange);
		}
		if (has_destructor()) {
			irCtx->Error(
			    "Destructor is not allowed in default implementations, but only in the original type, if the type is a struct type",
			    destructorDefinition->fileRange);
		}
	} else {
		irCtx->Error("Implementing skills is not supported for now", fileRange);
	}
}

void DoSkill::define_members(ir::Ctx* irCtx) {
	parent           = ir::MethodParent::create_do_skill(doneSkill);
	auto parentState = get_state_for(parent);
	if (has_default_constructor()) {
		defaultConstructor->define(parentState->defaultConstructor, irCtx);
	}
	if (has_copy_constructor()) {
		copyConstructor->define(parentState->copyConstructor, irCtx);
	}
	if (has_copy_assignment()) {
		copyAssignment->define(parentState->copyAssignment, irCtx);
	}
	if (has_move_constructor()) {
		moveConstructor->define(parentState->moveConstructor, irCtx);
	}
	if (has_move_assignment()) {
		moveAssignment->define(parentState->moveAssignment, irCtx);
	}
	if (has_destructor()) {
		destructorDefinition->define(parentState->destructor, irCtx);
	}
	for (auto* conv : convertorDefinitions) {
		MethodState state(parent);
		conv->define(state, irCtx);
		parentState->convertors.push_back(state);
	}
	for (auto* cons : constructorDefinitions) {
		MethodState state(parent);
		cons->define(state, irCtx);
		parentState->constructors.push_back(state);
	}
	for (auto* memDef : memberDefinitions) {
		MethodState state(parent);
		memDef->define(state, irCtx);
		parentState->allMethods.push_back(state);
	}
	for (auto* oprDef : operatorDefinitions) {
		MethodState state(parent);
		oprDef->define(state, irCtx);
		parentState->operators.push_back(state);
	}
}

void DoSkill::emit_members(ir::Ctx* irCtx) {
	parent           = ir::MethodParent::create_do_skill(doneSkill);
	auto parentState = get_state_for(parent);
	if (defaultConstructor) {
		(void)defaultConstructor->emit(parentState->defaultConstructor, irCtx);
	}
	if (copyConstructor) {
		(void)copyConstructor->emit(parentState->copyConstructor, irCtx);
	}
	if (moveConstructor) {
		(void)moveConstructor->emit(parentState->moveConstructor, irCtx);
	}
	if (copyAssignment) {
		(void)copyAssignment->emit(parentState->copyAssignment, irCtx);
	}
	if (moveAssignment) {
		(void)moveAssignment->emit(parentState->moveAssignment, irCtx);
	}
	for (usize i = 0; i < constructorDefinitions.size(); i++) {
		(void)constructorDefinitions[i]->emit(parentState->constructors[i], irCtx);
	}
	for (usize i = 0; i < convertorDefinitions.size(); i++) {
		(void)convertorDefinitions[i]->emit(parentState->convertors[i], irCtx);
	}
	for (usize i = 0; i < memberDefinitions.size(); i++) {
		(void)memberDefinitions[i]->emit(parentState->allMethods[i], irCtx);
	}
	for (usize i = 0; i < operatorDefinitions.size(); i++) {
		(void)operatorDefinitions[i]->emit(parentState->operators[i], irCtx);
	}
	if (destructorDefinition) {
		(void)destructorDefinition->emit(parentState->destructor, irCtx);
	}
	// TODO - Member function call tree analysis
}

Json DoSkill::to_json() const {
	return Json()._("isDefault", isDefaultSkill)._("targetType", targetType->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
