#include "./define_skill.hpp"
#include "../IR/types/void.hpp"
#include "./skill.hpp"
#include "./types/generic_abstract.hpp"

namespace qat::ast {

void SkillTypeDefinition::update_dependencies(ir::EmitPhase phase, ir::DependType depend, ir::EntityState* ent,
                                              EmitCtx* ctx) {
	if (defineChecker != nullptr) {
		UPDATE_DEPS(defineChecker);
	}
	UPDATE_DEPS(type);
}

void SkillMethod::update_dependencies(ir::EmitPhase phase, ir::DependType depend, ir::EntityState* ent, EmitCtx* ctx) {
	if (givenType != nullptr) {
		UPDATE_DEPS(givenType);
	}
	for (auto& arg : arguments) {
		if (arg->get_type() != nullptr) {
			UPDATE_DEPS(arg->get_type());
		}
	}
	if (defineChecker != nullptr) {
		UPDATE_DEPS(defineChecker);
	}
}

void DefineSkill::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	if (is_generic()) {
		mod->entity_name_check(irCtx, name, ir::EntityType::genericSkill);
		entityState = mod->add_entity(name, ir::EntityType::genericSkill, this, ir::EmitPhase::phase_1);
	} else {
		mod->entity_name_check(irCtx, name, ir::EntityType::skill);
		entityState                 = mod->add_entity(name, ir::EntityType::skill, this, ir::EmitPhase::phase_2);
		entityState->phaseToPartial = ir::EmitPhase::phase_1;
	}
}

void DefineSkill::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, parent);
	for (auto* gen : generics) {
		gen->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	for (auto& def : typeDefinitions) {
		def.update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	for (auto& fn : methods) {
		fn.update_dependencies(is_generic() ? ir::EmitPhase::phase_1 : ir::EmitPhase::phase_2, ir::DependType::complete,
		                       entityState, ctx);
	}
}

void DefineSkill::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	if (is_generic()) {
		auto emitCtx = EmitCtx::get(irCtx, mod);
		for (auto* gen : generics) {
			gen->emit(emitCtx);
		}
		genericSkill = ir::GenericSkill::create(name, mod, generics, this, genericConstraint,
		                                        emitCtx->get_visibility_info(visibSpec));
	} else {
		if (phase == ir::EmitPhase::phase_1) {
			resultSkill = create_skill({}, mod, irCtx);
			create_type_definitions(resultSkill, mod, irCtx);
		} else if (phase == ir::EmitPhase::phase_2) {
			create_methods(resultSkill, mod, irCtx);
		}
	}
}

ir::Skill* DefineSkill::create_skill(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* parent, ir::Ctx* irCtx) {
	Vec<ir::GenericArgument*> genericsIR;
	for (auto* gen : generics) {
		if (not gen->isSet()) {
			if (gen->is_typed()) {
				irCtx->Error("No type is set for the generic type parameter " + irCtx->color(gen->get_name().value) +
				                 " and there is no default type provided",
				             gen->get_range());
			} else if (gen->is_prerun()) {
				irCtx->Error("No value is set for the generic prerun expression parameter " +
				                 irCtx->color(gen->get_name().value) + " and there is no default expression provided",
				             gen->get_range());
			} else {
				irCtx->Error("Invalid generic kind", gen->get_range());
			}
		}
		genericsIR.push_back(gen->toIRGenericType());
	}
	auto skillResult =
	    ir::Skill::create(name, genericsIR, parent, EmitCtx::get(irCtx, parent)->get_visibility_info(visibSpec));
	if (genericSkill) {
		genericSkill->variants.push_back(ir::GenericVariant<ir::Skill>(skillResult, genericsToFill));
	}
	return skillResult;
}

void DefineSkill::create_type_definitions(ir::Skill* skill, ir::Mod* parent, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, parent)->with_skill(skill);
	for (auto& tyDef : typeDefinitions) {
		if (skill->has_definition(tyDef.name.value)) {
			ctx->Error("Found another type definition named " + ctx->color(tyDef.name.value) + " in the parent skill",
			           tyDef.name.range);
		}
		if (skill->has_any_prototype(tyDef.name.value)) {
			ctx->Error("Found method named " + ctx->color(tyDef.name.value) +
			               " in the parent skill. Please change name of this type definition",
			           tyDef.name.range);
		}
		if (tyDef.defineChecker) {
			auto defCond = tyDef.defineChecker->emit(ctx);
			if (not defCond->get_ir_type()->is_bool()) {
				irCtx->Error("Define conditions are required to be of type " + irCtx->color("bool") +
				                 ", but got an expression of type " + ctx->color(defCond->get_ir_type()->to_string()),
				             tyDef.defineChecker->fileRange);
			}
			if (not llvm::cast<llvm::ConstantInt>(defCond->get_llvm_constant())->getValue().getBoolValue()) {
				continue;
			}
		}
		auto typeRes = tyDef.type->emit(ctx);
		(void)ir::DefinitionType::create(tyDef.name, typeRes, {}, ir::TypeDefParent::from_skill(skill), parent,
		                                 ctx->get_visibility_info(tyDef.visibSpec));
	}
}

void DefineSkill::create_methods(ir::Skill* skill, ir::Mod* parent, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, parent)->with_skill(skill);
	for (auto& fn : methods) {
		if (skill->has_definition(fn.name.value)) {
			ctx->Error("Found a type definition named " + ctx->color(fn.name.value) + " in the parent skill",
			           fn.name.range);
		}
		if (skill->has_prototype(fn.name.value, method_kind_to_ir(fn.kind))) {
			ctx->Error("Found a " + method_kind_to_string(fn.kind) + " method with the same name " +
			               ctx->color(fn.name.value) + " in the parent skill",
			           fn.name.range);
		}
		if (skill->has_prototype(fn.name.value, ir::SkillMethodKind::STATIC)) {
			ctx->Error("Found a static method named " + ctx->color(fn.name.value) + " in the parent skill. A " +
			               ctx->color(method_kind_to_string(fn.kind)) +
			               " method cannot have the same name as a static method",
			           fn.name.range);
		}
		Vec<ir::SkillArg*> args;
		for (usize i = 0; i < fn.arguments.size(); i++) {
			auto& arg = fn.arguments[i];
			if (arg->is_variadic_arg() && (i != (fn.arguments.size() - 1))) {
				ctx->Error("Variadic argument should be the last argument", arg->get_name().range);
			}
			if (arg->is_member_arg()) {
				ctx->Error("Member arguments are not supported in skill methods", arg->get_name().range);
			}
			if (arg->is_variadic_arg()) {
				args.push_back(ir::SkillArg::create_variadic(arg->get_name()));
			} else {
				args.push_back(ir::SkillArg::create(ir::TypeInSkill::get(arg->get_type(), arg->get_type()->emit(ctx)),
				                                    arg->get_name(), arg->is_variable()));
			}
		}
		ir::TypeInSkill givenType = fn.givenType ? ir::TypeInSkill::get(fn.givenType, fn.givenType->emit(ctx))
		                                         : ir::TypeInSkill::get(nullptr, ir::VoidType::get(irCtx->llctx));
		if (fn.kind == SkillMethodKind::STATIC) {
			(void)ir::SkillMethod::create_static_method(skill, fn.name, givenType, std::move(args));
		} else if (fn.kind == SkillMethodKind::NORMAL || fn.kind == SkillMethodKind::VARIATION) {
			(void)ir::SkillMethod::create_method(skill, fn.name, fn.kind == SkillMethodKind::VARIATION, givenType,
			                                     std::move(args));
		}
	}
}

} // namespace qat::ast
