#include "./generic_named_type.hpp"
#include "../../IR/stdlib.hpp"
#include "../../show.hpp"
#include "../prerun/default.hpp"
#include "../types/prerun_generic.hpp"

namespace qat::ast {

void GenericNamedType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                           EmitCtx* ctx) {
	auto* mod     = ctx->mod;
	auto  reqInfo = ctx->get_access_info();
	if (relative != 0) {
		if (mod->has_nth_parent(relative)) {
			mod = mod->get_nth_parent(relative);
		}
	}
	auto entityName = names.back();
	if (names.size() > 1) {
		for (usize i = 0; i < (names.size() - 1); i++) {
			auto split = names.at(i);
			if (split.value == "std" && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			} else if (relative == 0 && i == 0 && mod->has_entity_with_name(split.value)) {
				ent->addDependency(ir::EntityDependency{mod->get_entity(split.value),
				                                        expect.value_or(ir::DependType::complete), phase});
				break;
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
				               ctx->color(mod->get_full_name()) + " or any of its submodules",
				           fileRange);
			}
		}
	}
	if (mod->has_entity_with_name(entityName.value)) {
		ent->addDependency(ir::EntityDependency{mod->get_entity(entityName.value), ir::DependType::complete, phase});
	}
}

Maybe<ir::Type*> handle_generic_named_type(ir::Mod* mod, ir::Block* curr, Identifier entityName, Vec<Identifier> names,
                                           Vec<FillGeneric*> genericTypes, AccessInfo reqInfo, FileRange fileRange,
                                           EmitCtx* ctx) {
	if (mod->has_generic_struct_type(entityName.value, reqInfo) ||
	    mod->has_brought_generic_struct_type(entityName.value, reqInfo) ||
	    mod->has_generic_struct_type_in_imports(entityName.value, reqInfo).first) {
		auto* genericCoreTy = mod->get_generic_struct_type(entityName.value, reqInfo);
		if (!genericCoreTy->get_visibility().is_accessible(reqInfo)) {
			auto fullName = Identifier::fullName(names);
			ctx->Error("Generic core type " + ctx->color(fullName.value) + " is not accessible here", fullName.range);
		}
		SHOW("Added mention for generic")
		genericCoreTy->add_mention(entityName.range);
		Vec<ir::GenericToFill*> types;
		if (genericTypes.empty()) {
			SHOW("Checking if all generic abstracts have defaults")
			if (!genericCoreTy->allTypesHaveDefaults()) {
				ctx->Error(
				    "Not all generic parameters in this type have a default value associated with it, and hence the type parameter list cannot be empty. Use " +
				        ctx->color("default") + " to use the default type or value of the generic parameter.",
				    fileRange);
			}
			SHOW("Check complete")
		} else if (genericCoreTy->getTypeCount() != genericTypes.size()) {
			ctx->Error(
			    "Generic core type " + ctx->color(genericCoreTy->get_name().value) + " has " +
			        ctx->color(std::to_string(genericCoreTy->getTypeCount())) + " generic parameters. But " +
			        ((genericCoreTy->getTypeCount() > genericTypes.size()) ? "only " : "") +
			        ctx->color(std::to_string(genericTypes.size())) +
			        " values were provided. Not all generic parameters have default values, and hence the number of values provided must match. Use " +
			        ctx->color("default") + " to use the default type or value of the generic parameter.",
			    fileRange);
		} else {
			for (usize i = 0; i < genericTypes.size(); i++) {
				if (genericTypes.at(i)->is_prerun()) {
					auto* gen = genericTypes.at(i);
					if (gen->is_prerun() && (gen->as_prerun()->nodeType() == NodeType::PRERUN_DEFAULT)) {
						((ast::PrerunDefault*)(gen->as_prerun()))->setGenericAbstract(genericCoreTy->getGenericAt(i));
					} else if (genericCoreTy->getGenericAt(i)->as_typed() &&
					           (genericCoreTy->getGenericAt(i)->as_prerun()->getType() != nullptr)) {
						if (gen->as_prerun()->has_type_inferrance()) {
							gen->as_prerun()->as_type_inferrable()->set_inference_type(
							    genericCoreTy->getGenericAt(i)->as_prerun()->getType());
						}
					}
				}
				types.push_back(genericTypes.at(i)->toFill(ctx));
			}
		}
		SHOW("Filling generics")
		auto* tyRes = genericCoreTy->fill_generics(types, ctx->irCtx, fileRange);
		SHOW("Filled generics: " << tyRes->is_struct())
		SHOW("Generic filled: " << tyRes->to_string())
		SHOW("  with llvm type: " << (tyRes->get_llvm_type()->isStructTy()
		                                  ? tyRes->get_llvm_type()->getStructName().str()
		                                  : ""))
		if (curr) {
			curr->set_active(ctx->irCtx->builder);
		}
		SHOW("Set old mod")
		return tyRes;
	} else if (mod->has_generic_type_def(entityName.value, reqInfo) ||
	           mod->has_brought_generic_type_def(entityName.value, reqInfo) ||
	           mod->has_generic_type_def_in_imports(entityName.value, reqInfo).first) {
		auto* genericTypeDef = mod->get_generic_type_def(entityName.value, reqInfo);
		if (!genericTypeDef->get_visibility().is_accessible(reqInfo)) {
			auto fullName = Identifier::fullName(names);
			ctx->Error("Generic type definition " + ctx->color(fullName.value) + " is not accessible here",
			           fullName.range);
		}
		genericTypeDef->add_mention(entityName.range);
		Vec<ir::GenericToFill*> types;
		if (genericTypes.empty()) {
			SHOW("Checking if all generic abstracts have defaults")
			if (!genericTypeDef->all_generics_have_defaults()) {
				ctx->Error(
				    "Not all generic parameters in this type have a default value associated with it, and hence the generic parameter list cannot be empty. Use " +
				        ctx->color("default") + " to use the default type or value of the generic parameter.",
				    fileRange);
			}
			SHOW("Check complete")
		} else if (genericTypeDef->get_generic_count() != genericTypes.size()) {
			ctx->Error(
			    "Generic type definition " + ctx->color(genericTypeDef->get_name().value) + " has " +
			        ctx->color(std::to_string(genericTypeDef->get_generic_count())) + " generic parameters. But " +
			        ((genericTypeDef->get_generic_count() > genericTypes.size()) ? "only " : "") +
			        ctx->color(std::to_string(genericTypes.size())) +
			        " values were provided. Not all generic parameters have default values, and hence the number of values provided must match. Use " +
			        ctx->color("default") + " to use the default type or value of the generic parameter.",
			    fileRange);
		} else {
			for (usize i = 0; i < genericTypes.size(); i++) {
				if (genericTypes.at(i)->is_prerun()) {
					auto* gen = genericTypes.at(i);
					if (gen->is_prerun() && (gen->as_prerun()->nodeType() == NodeType::PRERUN_DEFAULT)) {
						((ast::PrerunDefault*)(gen->as_prerun()))
						    ->setGenericAbstract(genericTypeDef->get_generic_at(i));
					} else if (genericTypeDef->get_generic_at(i)->as_typed() &&
					           (genericTypeDef->get_generic_at(i)->as_prerun()->getType() != nullptr)) {
						if (gen->as_prerun()->has_type_inferrance()) {
							gen->as_prerun()->as_type_inferrable()->set_inference_type(
							    genericTypeDef->get_generic_at(i)->as_prerun()->getType());
						}
					}
				}
				types.push_back(genericTypes.at(i)->toFill(ctx));
			}
		}
		auto tyRes = genericTypeDef->fill_generics(types, ctx->irCtx, fileRange);
		if (curr) {
			curr->set_active(ctx->irCtx->builder);
		}
		return tyRes;
	}
	return None;
}

ir::Type* GenericNamedType::emit(EmitCtx* ctx) {
	SHOW("Generic named type START")
	auto* mod     = ctx->mod;
	auto  reqInfo = ctx->get_access_info();
	if (relative != 0) {
		if (mod->has_nth_parent(relative)) {
			mod = mod->get_nth_parent(relative);
		}
	}
	auto entityName = names.back();
	if (names.size() > 1) {
		for (usize i = 0; i < (names.size() - 1); i++) {
			auto split = names.at(i);
			if (split.value == "std" && ir::StdLib::is_std_lib_found()) {
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
				               ctx->color(mod->get_full_name()) + " or any of its submodules",
				           fileRange);
			}
		}
		entityName = names.back();
	}
	auto* fun       = ctx->has_fn() ? ctx->get_fn() : nullptr;
	auto* curr      = fun ? fun->get_block() : nullptr;
	auto  handleRes = handle_generic_named_type(mod, curr, entityName, names, genericTypes, reqInfo, fileRange, ctx);
	if (handleRes.has_value()) {
		return handleRes.value();
	} else if (mod->has_generic_function(entityName.value, reqInfo) ||
	           mod->has_brought_generic_function(entityName.value, reqInfo) ||
	           mod->has_generic_function_in_imports(entityName.value, reqInfo).first) {
		ctx->Error(ctx->color(entityName.value) + " is a generic function and hence cannot be used as a type",
		           fileRange);
	} else {
		// FIXME - Support static members of generic types
		ctx->Error("No generic type named " + ctx->color(Identifier::fullName(names).value) +
		               " found in the current scope",
		           fileRange);
	}
	return nullptr;
}

Json GenericNamedType::to_json() const {
	Vec<JsonValue> nameJs;
	for (auto const& nam : names) {
		nameJs.push_back(JsonValue(nam));
	}
	Vec<JsonValue> typs;
	for (auto* typ : genericTypes) {
		typs.push_back(typ->to_json());
	}
	return Json()._("typeKind", "genericNamed")._("names", nameJs)._("types", typs)._("fileRange", fileRange);
}

String GenericNamedType::to_string() const {
	auto result = Identifier::fullName(names).value + ":[";
	for (usize i = 0; i < genericTypes.size(); i++) {
		result += genericTypes.at(i)->to_string();
		if (i != (genericTypes.size() - 1)) {
			result += ", ";
		}
	}
	result += "]";
	return result;
}

} // namespace qat::ast