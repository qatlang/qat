#include "./do_skill.hpp"
#include "../IR/skill.hpp"
#include "./constructor.hpp"
#include "./convertor.hpp"
#include "./destructor.hpp"
#include "./method.hpp"
#include "./operator_function.hpp"
#include "./type_definition.hpp"
#include "context.hpp"

#include <unordered_set>

namespace qat::ast {

void DoSkill::create_entity(ir::Mod* parent, ir::Ctx* irCtx) {
	entityState =
	    parent->add_entity(None, isDefaultSkill ? ir::EntityType::defaultDoneSkill : ir::EntityType::doneSkill, this,
	                       ir::EmitPhase::phase_3);
	entityState->phaseToPartial = ir::EmitPhase::phase_2;
	for (auto memFn : methodDefinitions) {
		memFn->prototype->add_to_parent(entityState, irCtx);
	}
}

void DoSkill::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, parent);
	if (name.has_value()) {
		name.value().update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	targetType->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	for (auto* def : typeDefinitions) {
		def->update_dependencies_for_parent(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	if (defaultConstructor) {
		defaultConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete,
		                                                   entityState, ctx);
		defaultConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	if (copyConstructor) {
		copyConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
		                                                ctx);
		copyConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	if (moveConstructor) {
		moveConstructor->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
		                                                ctx);
		moveConstructor->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	if (copyAssignment) {
		copyAssignment->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
		                                               ctx);
		copyAssignment->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	if (moveAssignment) {
		moveAssignment->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState,
		                                               ctx);
		moveAssignment->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	if (destructorDefinition) {
		destructorDefinition->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	for (auto cons : constructorDefinitions) {
		cons->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		cons->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	for (auto conv : convertorDefinitions) {
		conv->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		conv->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	for (auto memFn : methodDefinitions) {
		memFn->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		memFn->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
	for (auto opr : operatorDefinitions) {
		opr->prototype->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		opr->update_dependencies(ir::EmitPhase::phase_3, ir::DependType::complete, entityState, ctx);
	}
}

void DoSkill::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) {
	if (phase == ir::EmitPhase::phase_1) {
		define_done_skill(parent, irCtx);
		define_types(doneSkill, parent, irCtx);
	} else if (phase == ir::EmitPhase::phase_2) {
		define_members(irCtx);
	} else if (phase == ir::EmitPhase::phase_3) {
		emit_members(irCtx);
	}
}

void DoSkill::define_types(ir::DoneSkill* skillImp, ir::Mod* mod, ir::Ctx* irCtx) {
	auto methodParent = ir::MethodParent::create_do_skill(skillImp);
	auto parentState  = get_state_for(methodParent);
	parentState->definitions.reserve(typeDefinitions.size());
	for (auto* def : typeDefinitions) {
		auto tyState = TypeInParentState{.parent = methodParent, .isParentSkill = false};
		def->create_type_in_parent(tyState, mod, irCtx);
		parentState->definitions.push_back(tyState);
	}
}

void DoSkill::define_done_skill(ir::Mod* mod, ir::Ctx* irCtx) {
	auto* target = targetType->emit(EmitCtx::get(irCtx, mod));
	if (target->is_region() || target->is_ref() || target->is_typed() || target->is_function() || target->is_void() ||
	    target->is_poly()) {
		irCtx->Error("Creating a default implementation for " + irCtx->color(target->to_string()) + " is not allowed",
		             fileRange);
	}
	if (isDefaultSkill) {
		doneSkill = ir::DoneSkill::create_extension(mod, fileRange, target, targetType->fileRange);
		if (has_copy_constructor()) {
			irCtx->Error("Copy constructor is not allowed in type extensions, but only in the original type",
			             copyConstructor->fileRange);
		}
		if (has_copy_assignment()) {
			irCtx->Error("Copy assignment is not allowed in type extensions, but only in the original type",
			             copyAssignment->fileRange);
		}
		if (has_move_constructor()) {
			irCtx->Error("Move constructor is not allowed in type extensions, but only in the original type",
			             moveConstructor->fileRange);
		}
		if (has_move_assignment()) {
			irCtx->Error("Move assignment is not allowed in type extensions, but only in the original type",
			             moveAssignment->fileRange);
		}
		if (has_destructor()) {
			irCtx->Error("Destructor is not allowed in type extensions, but only in the original type",
			             destructorDefinition->fileRange);
		}
	} else {
		doneSkill = ir::DoneSkill::create_normal(mod, name.value().find_skill(EmitCtx::get(irCtx, mod)), fileRange,
		                                         target, targetType->fileRange);
		if (not convertorDefinitions.empty()) {
			Vec<ir::QatError> errors;
			for (auto* conv : convertorDefinitions) {
				errors.push_back(ir::QatError("Convertors are not allowed in skill implementations", conv->fileRange));
			}
			irCtx->Errors(errors);
		}
		if (not constructorDefinitions.empty()) {
			Vec<ir::QatError> errors;
			for (auto* cons : constructorDefinitions) {
				errors.push_back(
				    ir::QatError("Constructors are not allowed in skill implementations", cons->fileRange));
			}
			irCtx->Errors(errors);
		}
		if (not operatorDefinitions.empty()) {
			Vec<ir::QatError> errors;
			for (auto* op : operatorDefinitions) {
				errors.push_back(ir::QatError("Operators are not allowed in skill implementations", op->fileRange));
			}
			irCtx->Errors(errors);
		}
		if (has_copy_constructor()) {
			irCtx->Error("Copy constructor is not allowed in skill implementations, but only in the original type",
			             copyConstructor->fileRange);
		}
		if (has_copy_assignment()) {
			irCtx->Error("Copy assignment is not allowed in skill implementations, but only in the original type",
			             copyAssignment->fileRange);
		}
		if (has_move_constructor()) {
			irCtx->Error("Move constructor is not allowed in skill implementations, but only in the original type",
			             moveConstructor->fileRange);
		}
		if (has_move_assignment()) {
			irCtx->Error("Move assignment is not allowed in skill implementations, but only in the original type",
			             moveAssignment->fileRange);
		}
		if (has_destructor()) {
			irCtx->Error("Destructor is not allowed in skill implementations, but only in the original type",
			             destructorDefinition->fileRange);
		}
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
	std::unordered_set<ir::SkillMethod*> skillMethods;
	if (doneSkill->is_normal_skill()) {
		skillMethods = std::unordered_set<ir::SkillMethod*>(doneSkill->get_skill()->prototypes.begin(),
		                                                    doneSkill->get_skill()->prototypes.end());
	}
	for (auto* memDef : methodDefinitions) {
		MethodState state(parent);
		memDef->define(state, irCtx);
		parentState->allMethods.push_back(state);
		skillMethods.erase(state.result->get_skill_method());
	}
	if (not skillMethods.empty()) {
		String methodStr;
		for (auto it : skillMethods) {
			methodStr += it->to_string() + "\n";
		}
		irCtx->Error("The following methods from the skill " + irCtx->color(doneSkill->get_skill()->get_full_name()) +
		                 " are missing from this implementation:\n" + methodStr,
		             fileRange);
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
	for (usize i = 0; i < methodDefinitions.size(); i++) {
		(void)methodDefinitions[i]->emit(parentState->allMethods[i], irCtx);
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
