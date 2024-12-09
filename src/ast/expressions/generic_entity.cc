#include "./generic_entity.hpp"
#include "../../IR/stdlib.hpp"
#include "../prerun/default.hpp"
#include "../types/generic_named_type.hpp"
#include "../types/prerun_generic.hpp"

namespace qat::ast {

void GenericEntity::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                        EmitCtx* ctx) {
	auto* mod     = ctx->mod;
	auto  reqInfo = ctx->get_access_info();
	if (relative != 0) {
		if (mod->has_nth_parent(relative)) {
			mod = mod->get_nth_parent(relative);
		} else {
			ctx->irCtx->Error("The current scope does not have " + ctx->color(std::to_string(relative)) +
			                      " parents. Please check the logic",
			                  fileRange);
		}
	}
	auto entityName = names.back();
	if (names.size() > 1) {
		for (usize i = 0; i < (names.size() - 1); i++) {
			auto split = names.at(i);
			if (relative == 0 && split.value == "std" && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			} else if (relative == 0 && i == 0 && mod->has_entity_with_name(split.value)) {
				ent->addDependency(
				    ir::EntityDependency{mod->get_entity(split.value), dep.value_or(ir::DependType::complete), phase});
				break;
			}
			if (mod->has_lib(split.value, reqInfo) || mod->has_brought_lib(split.value, reqInfo) ||
			    mod->has_lib_in_imports(split.value, reqInfo).first) {
				mod = mod->get_lib(split.value, reqInfo);
			} else if (mod->has_brought_mod(split.value, reqInfo) ||
			           mod->has_brought_mod_in_imports(split.value, reqInfo).first) {
				mod = mod->get_brought_mod(split.value, reqInfo);
			} else {
				ctx->irCtx->Error("No lib named " + ctx->irCtx->color(split.value) + " found inside " +
				                      ctx->irCtx->color(mod->get_full_name()),
				                  fileRange);
			}
		}
	}
	if (mod->has_entity_with_name(entityName.value)) {
		ent->addDependency(
		    ir::EntityDependency{mod->get_entity(entityName.value), dep.value_or(ir::DependType::complete), phase});
	}
}

ir::Value* GenericEntity::emit(EmitCtx* ctx) {
	auto* mod     = ctx->mod;
	auto  reqInfo = ctx->get_access_info();
	if (relative != 0) {
		if (mod->has_nth_parent(relative)) {
			mod = mod->get_nth_parent(relative);
		} else {
			ctx->Error("The current scope does not have " + ctx->color(std::to_string(relative)) +
			               " parents. Please check the logic",
			           fileRange);
		}
	}
	auto entityName = names.back();
	if (names.size() > 1) {
		for (usize i = 0; i < (names.size() - 1); i++) {
			auto split = names.at(i);
			if (relative == 0 && split.value == "std" && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			}
			if (mod->has_lib(split.value, reqInfo) || mod->has_brought_lib(split.value, reqInfo) ||
			    mod->has_lib_in_imports(split.value, reqInfo).first) {
				mod = mod->get_lib(split.value, reqInfo);
				mod->add_mention(split.range);
			} else if (mod->has_brought_mod(split.value, reqInfo) ||
			           mod->has_brought_mod_in_imports(split.value, reqInfo).first) {
				mod = mod->get_brought_mod(split.value, reqInfo);
				mod->add_mention(split.range);
			} else {
				ctx->Error("No lib named " + ctx->color(split.value) + " found inside " +
				               ctx->color(mod->get_full_name()),
				           fileRange);
			}
		}
	}
	auto* oldFn = ctx->get_fn();
	auto* curr  = ctx->has_fn() ? ctx->get_fn()->get_block() : nullptr;
	if (mod->has_generic_function(entityName.value, reqInfo) ||
	    mod->has_brought_generic_function(entityName.value, ctx->get_access_info()) ||
	    mod->has_generic_function_in_imports(entityName.value, reqInfo).first) {
		auto* genericFn = mod->get_generic_function(entityName.value, reqInfo);
		if (!genericFn->get_visibility().is_accessible(ctx->get_access_info())) {
			auto fullName = Identifier::fullName(names);
			ctx->Error("Generic function " + ctx->color(fullName.value) + " is not accessible here", fullName.range);
		}
		if (genericTypes.empty()) {
			SHOW("Checking if all generic abstracts have defaults")
			if (!genericFn->all_generics_have_default()) {
				ctx->Error(
				    "Not all generic parameters in this function have a default type associated with it, and hence the generic values can't be empty. Use " +
				        ctx->color("default") + " to use the default type or value of the generic parameter.",
				    fileRange);
			}
			SHOW("Check complete")
		} else if (genericFn->getTypeCount() != genericTypes.size()) {
			ctx->Error(
			    "Generic function " + ctx->color(genericFn->get_name().value) + " has " +
			        ctx->color(std::to_string(genericFn->getTypeCount())) + " generic parameters. But " +
			        ((genericFn->getTypeCount() > genericTypes.size()) ? "only " : "") +
			        ctx->color(std::to_string(genericTypes.size())) +
			        " values were provided. Not all generic parameters have default values, and hence the number of values provided must match. Use " +
			        ctx->color("default") + " to use the default type or value of the generic parameter.",
			    fileRange);
		}
		SHOW("About to create toFill")
		Vec<ir::GenericToFill*> types;
		for (usize i = 0; i < genericTypes.size(); i++) {
			auto* gen = genericTypes.at(i);
			if (gen->is_prerun() && (gen->as_prerun()->nodeType() == NodeType::PRERUN_DEFAULT)) {
				SHOW("Generic is prerun and prerun generic is default expression")
				((ast::PrerunDefault*)(genericTypes.at(i)->as_prerun()))
				    ->setGenericAbstract(genericFn->getGenericAt(i));
			} else if (genericFn->getGenericAt(i)->is_prerun() &&
			           (genericFn->getGenericAt(i)->as_prerun()->getType() != nullptr)) {
				SHOW("Generic abstract is prerun and has valid type")
				if (gen->as_prerun()->has_type_inferrance()) {
					gen->as_prerun()->as_type_inferrable()->set_inference_type(
					    genericFn->getGenericAt(i)->as_prerun()->getType());
				}
			}
			types.push_back(genericTypes.at(i)->toFill(ctx));
		}
		SHOW("Calling fill_generics")
		auto* fnRes = genericFn->fill_generics(types, ctx->irCtx, fileRange);
		SHOW("fill_generics completed")
		if (curr) {
			curr->set_active(ctx->irCtx->builder);
		}
		return fnRes;
	} else {
		auto handleRes = handle_generic_named_type(mod, curr, entityName, names, genericTypes, reqInfo, fileRange, ctx);
		if (handleRes.has_value()) {
			return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(handleRes.value()));
		} else {
			// FIXME - Support static members of generic types
			auto fullName = Identifier::fullName(names);
			ctx->Error("No generic function or type named " + ctx->color(fullName.value) +
			               " found in the current scope",
			           fullName.range);
		}
	}
	return nullptr;
}

Json GenericEntity::to_json() const {
	Vec<JsonValue> namesJs;
	for (auto const& nam : names) {
		namesJs.push_back(JsonValue(nam));
	}
	Vec<JsonValue> typs;
	for (auto* typ : genericTypes) {
		typs.push_back(typ->to_json());
	}
	return Json()._("nodeType", "genericEntity")._("names", namesJs)._("types", typs)._("fileRange", fileRange);
}

} // namespace qat::ast