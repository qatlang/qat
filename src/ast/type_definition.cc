#include "./type_definition.hpp"
#include "./expression.hpp"
#include "./member_parent_like.hpp"
#include "./types/generic_abstract.hpp"
#include "./types/prerun_generic.hpp"
#include "./types/typed_generic.hpp"

namespace qat::ast {

void TypeDefinition::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	typeSize = subType->get_type_bitsize(EmitCtx::get(irCtx, mod));
	mod->entity_name_check(irCtx, name, is_generic() ? ir::EntityType::genericTypeDef : ir::EntityType::typeDefinition);
	entityState = mod->add_entity(name, is_generic() ? ir::EntityType::genericTypeDef : ir::EntityType::typeDefinition,
	                              this, ir::EmitPhase::phase_1);
}

void TypeDefinition::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("Creating emitctx")
	auto ctx = EmitCtx::get(irCtx, mod);
	if (checker) {
		checker->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	SHOW("Checking generic")
	if (is_generic()) {
		SHOW("Type def is generic. Updating deps")
		for (auto gen : generics) {
			if (gen->is_prerun() && gen->as_prerun()->hasDefault()) {
				gen->as_prerun()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete,
				                                                       entityState, ctx);
			} else if (gen->is_typed() && gen->as_typed()->hasDefault()) {
				gen->as_typed()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete,
				                                                      entityState, ctx);
			}
		}
	}
	SHOW("Updating dependencies of subtype")
	subType->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
}

void TypeDefinition::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	if (checker) {
		auto* checkRes = checker->emit(emitCtx);
		if (checkRes->get_ir_type()->is_bool()) {
			checkResult = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
			if (not checkResult.value()) {
				// TODO - ADD THIS AS DEAD CODE IN CODE INFO
				return;
			}
		} else {
			irCtx->Error("The condition for this type definition should be of " + irCtx->color("bool") +
			                 " type. Got an expression of type " + irCtx->color(checkRes->get_ir_type()->to_string()),
			             checker->fileRange);
		}
	}
	if (is_generic()) {
		genericTypeDefinition = ir::GenericDefinitionType::create(
		    name, generics, constraint ? Maybe<ast::PrerunExpression*>(constraint) : None, this, mod,
		    emitCtx->get_visibility_info(visibSpec));
	} else {
		create_type(mod, irCtx);
	}
}

bool TypeDefinition::is_generic() const { return not generics.empty(); }

void TypeDefinition::set_variant_name(const String& name) const { variantName = name; }

void TypeDefinition::unset_variant_name() const { variantName = None; }

void TypeDefinition::create_type(ir::Mod* mod, ir::Ctx* irCtx) const {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	emitCtx->name_check_in_module(name, is_generic() ? "generic type definition" : "type definition",
	                              is_generic() ? Maybe<u64>(genericTypeDefinition->get_id()) : None);
	auto dTyName = name;
	if (is_generic()) {
		dTyName = Identifier(variantName.value(), name.range);
	}
	Vec<ir::GenericArgument*> genericsIR;
	for (auto* gen : generics) {
		if (not gen->isSet()) {
			if (gen->is_typed()) {
				irCtx->Error("No type is set for the generic type " + irCtx->color(gen->get_name().value) +
				                 " and there is no default type provided",
				             gen->get_range());
			} else if (gen->as_typed()) {
				irCtx->Error("No value is set for the generic prerun expression " +
				                 irCtx->color(gen->get_name().value) + " and there is no default expression provided",
				             gen->get_range());
			} else {
				irCtx->Error("Invalid generic kind", gen->get_range());
			}
		}
		genericsIR.push_back(gen->toIRGenericType());
	}
	if (is_generic()) {
		irCtx->get_active_generic().generics = genericsIR;
	}
	SHOW("Type definition " << dTyName.value)
	typeDefinition = ir::DefinitionType::create(dTyName, subType->emit(emitCtx), genericsIR, None, mod,
	                                            emitCtx->get_visibility_info(visibSpec));
}

void TypeDefinition::create_type_in_parent(TypeInParentState& state, ir::Mod* mod, ir::Ctx* irCtx) const {
	if (state.isParentSkill) {
		auto sk = (ir::Skill*)state.parent;
		if (sk->has_definition(name.value)) {
			irCtx->Error("Found another type definition in " + irCtx->color(sk->get_full_name()) +
			                 " with the same name",
			             name.range);
		} else if (sk->has_any_prototype(name.value)) {
			irCtx->Error("Found a method in " + irCtx->color(sk->get_full_name()) + " with the same name", name.range);
		}
	} else if (((ir::MethodParent*)state.parent)->is_done_skill()) {
		auto sk = ((ir::MethodParent*)state.parent)->as_done_skill();
		if (visibSpec.has_value() && visibSpec.value().kind != VisibilityKind::pub) {
			irCtx->Error(
			    "Type definitions in implementations are always public, please remove the visibility specifier",
			    visibSpec.value().range);
		}
		if (sk->has_normal_method(name.value)) {
			irCtx->Error("Found a method in " + irCtx->color(sk->get_full_name()) + " with the same name", name.range);
		} else if (sk->has_variation_method(name.value)) {
			irCtx->Error("Found a variation method in " + irCtx->color(sk->get_full_name()) + " with the same name",
			             name.range);
		} else if (sk->has_valued_method(name.value)) {
			irCtx->Error("Found a value method in " + irCtx->color(sk->get_full_name()) + " with the same name",
			             name.range);
		} else if (sk->has_static_method(name.value)) {
			irCtx->Error("Found a static function in " + irCtx->color(sk->get_full_name()) + " with the same name",
			             name.range);
		} else if (sk->has_definition(name.value)) {
			irCtx->Error("Found a type definition in " + irCtx->color(sk->get_full_name()) + " with the same name",
			             name.range);
		}
	} else {
		auto ex = ((ir::MethodParent*)state.parent)->as_expanded();
		if (ex->has_normal_method(name.value)) {
			irCtx->Error("Found a method in " + irCtx->color(ex->get_full_name()) + " with the same name", name.range);
		} else if (ex->has_variation(name.value)) {
			irCtx->Error("Found a variation method in " + irCtx->color(ex->get_full_name()) + " with the same name",
			             name.range);
		} else if (ex->has_valued_method(name.value)) {
			irCtx->Error("Found a value method in " + irCtx->color(ex->get_full_name()) + " with the same name",
			             name.range);
		} else if (ex->has_static_method(name.value)) {
			irCtx->Error("Found a static function in " + irCtx->color(ex->get_full_name()) + " with the same name",
			             name.range);
		} else if (ex->has_definition(name.value)) {
			irCtx->Error("Found a type definition in " + irCtx->color(ex->get_full_name()) + " with the same name",
			             name.range);
		}
	}
	if (state.isParentSkill) {
		auto emitCtx = EmitCtx::get(irCtx, mod)->with_skill((ir::Skill*)state.parent);
		ir::DefinitionType::create(name, subType->emit(emitCtx), {},
		                           ir::TypeDefParent::from_skill((ir::Skill*)state.parent), mod,
		                           emitCtx->get_visibility_info(visibSpec));
	} else {
		auto emitCtx = EmitCtx::get(irCtx, mod)->with_member_parent((ir::MethodParent*)state.parent);
		ir::DefinitionType::create(name, subType->emit(emitCtx), {},
		                           ir::TypeDefParent::from_method_parent((ir::MethodParent*)state.parent), mod,
		                           emitCtx->get_visibility_info(visibSpec));
	}
}

ir::DefinitionType* TypeDefinition::getDefinition() const { return typeDefinition; }

Json TypeDefinition::to_json() const {
	Vec<JsonValue> genJson;
	for (auto* gen : generics) {
		genJson.push_back(gen->to_json());
	}
	return Json()
	    ._("nodeType", "typeDefinition")
	    ._("name", name)
	    ._("subType", subType->to_json())
	    ._("hasGenerics", !generics.empty())
	    ._("generics", genJson)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
