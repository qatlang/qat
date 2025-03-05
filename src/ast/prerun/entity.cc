#include "./entity.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/flag.hpp"
#include "../../IR/types/region.hpp"
#include "../types/generic_abstract.hpp"
#include "../types/prerun_generic.hpp"
#include "../types/typed_generic.hpp"

namespace qat::ast {

void PrerunEntity::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                       EmitCtx* ctx) {
	auto mod     = ctx->mod;
	auto reqInfo = ctx->get_access_info();
	if (relative > 0) {
		if (mod->has_nth_parent(relative)) {
			mod = mod->get_nth_parent(relative);
		} else {
			ctx->Error("Module does not have " + ctx->color(std::to_string(relative)) + " parents", fileRange);
		}
	}
	for (usize i = 0; i < (identifiers.size() - 1); i++) {
		auto section = identifiers.at(i);
		if (relative == 0 && i == 0 && section.value == "std" && ir::StdLib::is_std_lib_found()) {
			mod = ir::StdLib::stdLib;
			continue;
		}
		if (mod->has_lib(section.value, reqInfo) || mod->has_brought_lib(section.value, reqInfo) ||
		    mod->has_lib_in_imports(section.value, reqInfo).first) {
			mod = mod->get_lib(section.value, reqInfo);
			mod->add_mention(section.range);
		} else if (mod->has_brought_mod(section.value, reqInfo) ||
		           mod->has_brought_mod_in_imports(section.value, reqInfo).first) {
			mod = mod->get_brought_mod(section.value, reqInfo);
			mod->add_mention(section.range);
		} else {
			ctx->Error("No module named " + ctx->color(section.value) + " found inside the current module", fileRange);
		}
	}
	auto entName = identifiers.back();
	if (mod->has_entity_with_name(entName.value)) {
		ent->addDependency(
		    ir::EntityDependency{mod->get_entity(entName.value), dep.value_or(ir::DependType::complete), phase});
	}
}

ir::PrerunValue* PrerunEntity::emit(EmitCtx* ctx) {
	SHOW("PrerunEntity")
	auto* mod  = ctx->mod;
	auto  name = identifiers.back();
	if (identifiers.size() == 1 && relative == 0) {
		if (ctx->has_pre_call_state()) {
			if (ctx->get_pre_call_state()->has_arg_with_name(name.value)) {
				return ctx->get_pre_call_state()->get_arg_value_for(name.value);
			} else if (ctx->get_pre_call_state()->get_block()->has_local(name.value)) {
				return ctx->get_pre_call_state()->get_block()->get_local(name.value);
			}
		}
		if (ctx->has_generic_with_name(identifiers[0].value)) {
			auto genAbs = ctx->get_generic_with_name(identifiers[0].value);
			if (genAbs->is_prerun()) {
				auto preGen = genAbs->as_prerun();
				if (not preGen->isSet()) {
					ctx->Error("The prerun generic parameter referred to by this symbol does not have a value",
					           fileRange);
				}
				return preGen->getPrerun();
			} else if (genAbs->is_typed()) {
				auto tyGen = genAbs->as_typed();
				if (not tyGen->isSet()) {
					ctx->Error("The typed generic parameter referred to by this symbol does not have a value",
					           fileRange);
				}
				return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, tyGen->get_type(), mod)->id,
				                            ir::TypedType::get(ctx->irCtx));
			} else {
				ctx->Error("Unsupported generic parameter kind", fileRange);
			}
		}
		SHOW("Has function " << (ctx->has_fn() ? "true" : "false"))
		if (ctx->has_fn() && ctx->get_fn()->has_generic_parameter(identifiers[0].value)) {
			SHOW("PrerunEntity: Has active function and generic parameter")
			auto* genVal = ctx->get_fn()->get_generic_parameter(identifiers[0].value);
			if (genVal->is_typed()) {
				return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, genVal->as_typed()->get_type(), mod)->id,
				                            ir::TypedType::get(ctx->irCtx));
			} else if (genVal->is_prerun()) {
				return genVal->as_prerun()->get_expression();
			} else {
				ctx->Error("Invalid generic kind", fileRange);
			}
		}
		SHOW("Checked for generic function")
		if (ctx->has_member_parent()) {
			if (ctx->get_member_parent()->is_expanded() && ctx->get_member_parent()->as_expanded()->is_generic() &&
			    ctx->get_member_parent()->as_expanded()->has_generic_parameter(identifiers[0].value)) {
				auto* genVal = ctx->get_member_parent()->as_expanded()->get_generic_parameter(identifiers[0].value);
				if (genVal->is_typed()) {
					return ir::PrerunValue::get(
					    ir::TypeInfo::create(ctx->irCtx, genVal->as_typed()->get_type(), mod)->id,
					    ir::TypedType::get(ctx->irCtx));
				} else if (genVal->is_prerun()) {
					return genVal->as_prerun()->get_expression();
				} else {
					ctx->Error("Invalid generic kind", fileRange);
				}
			}
			if (ctx->get_member_parent()->is_done_skill() && ctx->get_member_parent()->as_done_skill()->is_generic() &&
			    ctx->get_member_parent()->as_done_skill()->has_generic_parameter(identifiers[0].value)) {
				auto* genVal = ctx->get_member_parent()->as_done_skill()->get_generic_parameter(identifiers[0].value);
				if (genVal->is_typed()) {
					return ir::PrerunValue::get(
					    ir::TypeInfo::create(ctx->irCtx, genVal->as_typed()->get_type(), mod)->id,
					    ir::TypedType::get(ctx->irCtx));
				} else if (genVal->is_prerun()) {
					return genVal->as_prerun()->get_expression();
				} else {
					ctx->Error("Invalid generic kind", fileRange);
				}
			}
		}
		if (ctx->irCtx->has_active_generic()) {
			if (ctx->irCtx->has_generic_parameter_in_entity(identifiers[0].value)) {
				auto* genVal = ctx->irCtx->get_generic_parameter_from_entity(name.value);
				if (genVal->is_typed()) {
					return ir::PrerunValue::get(
					    ir::TypeInfo::create(ctx->irCtx, genVal->as_typed()->get_type(), mod)->id,
					    ir::TypedType::get(ctx->irCtx));
				} else if (genVal->is_prerun()) {
					return genVal->as_prerun()->get_expression();
				} else {
					ctx->Error("Invalid generic kind", fileRange);
				}
			}
		}
		if (ctx->has_fn() && ctx->get_fn()->is_method() &&
		    (((ir::Method*)ctx->get_fn())->get_parent_type()->is_expanded()) &&
		    ((ir::Method*)ctx->get_fn())
		        ->get_parent_type()
		        ->as_expanded()
		        ->has_generic_parameter(identifiers[0].value)) {
			// FIXME - Also check generic skills
			auto* genVal = ((ir::Method*)ctx->get_fn())
			                   ->get_parent_type()
			                   ->as_expanded()
			                   ->get_generic_parameter(identifiers[0].value);
			if (genVal->is_typed()) {
				return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, genVal->as_typed()->get_type(), mod)->id,
				                            ir::TypedType::get(ctx->irCtx));
			} else if (genVal->is_prerun()) {
				return genVal->as_prerun()->get_expression();
			} else {
				ctx->Error("Invalid generic kind", fileRange);
			}
		}
	} else {
		auto reqInfo = ctx->get_access_info();
		if (relative > 0) {
			if (mod->has_nth_parent(relative)) {
				mod = mod->get_nth_parent(relative);
			} else {
				ctx->Error("Module does not have " + ctx->color(std::to_string(relative)) + " parents", fileRange);
			}
		}
		for (usize i = 0; i < (identifiers.size() - 1); i++) {
			auto section = identifiers.at(i);
			if (relative == 0 && i == 0 && section.value == "std" && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			}
			if (mod->has_lib(section.value, reqInfo) || mod->has_brought_lib(section.value, reqInfo) ||
			    mod->has_lib_in_imports(section.value, reqInfo).first) {
				mod = mod->get_lib(section.value, reqInfo);
				mod->add_mention(section.range);
			} else if (mod->has_brought_mod(section.value, reqInfo) ||
			           mod->has_brought_mod_in_imports(section.value, reqInfo).first) {
				mod = mod->get_brought_mod(section.value, reqInfo);
				mod->add_mention(section.range);
			} else {
				ctx->Error("No module named " + ctx->color(section.value) + " found inside the current module",
				           fileRange);
			}
		}
	}
	auto reqInfo = ctx->get_access_info();
	if (mod->has_type_definition(name.value, reqInfo) || mod->has_brought_type_definition(name.value, reqInfo) ||
	    mod->has_type_definition_in_imports(name.value, reqInfo).first) {
		auto resTypeDef = mod->get_type_def(name.value, reqInfo);
		resTypeDef->add_mention(name.range);
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, resTypeDef, mod)->id,
		                            ir::TypedType::get(ctx->irCtx));
	} else if (mod->has_struct_type(name.value, reqInfo) || mod->has_brought_struct_type(name.value, reqInfo) ||
	           mod->has_struct_type_in_imports(name.value, reqInfo).first) {
		auto resCoreType = mod->get_struct_type(name.value, reqInfo);
		resCoreType->add_mention(name.range);
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, resCoreType, mod)->id,
		                            ir::TypedType::get(ctx->irCtx));
	} else if (mod->has_mix_type(name.value, reqInfo) || mod->has_brought_mix_type(name.value, reqInfo) ||
	           mod->has_mix_type_in_imports(name.value, reqInfo).first) {
		auto resMixType = mod->get_mix_type(name.value, reqInfo);
		resMixType->add_mention(name.range);
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, resMixType, mod)->id,
		                            ir::TypedType::get(ctx->irCtx));
	} else if (mod->has_choice_type(name.value, reqInfo) || mod->has_brought_choice_type(name.value, reqInfo) ||
	           mod->has_choice_type_in_imports(name.value, reqInfo).first) {
		auto resChoiceType = mod->get_choice_type(name.value, reqInfo);
		resChoiceType->add_mention(name.range);
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, resChoiceType, mod)->id,
		                            ir::TypedType::get(ctx->irCtx));
	} else if (mod->has_flag_type(name.value, reqInfo) || mod->has_brought_flag_type(name.value, reqInfo) ||
	           mod->has_flag_type_in_imports(name.value, reqInfo).first) {
		auto flagTy = mod->get_flag_type(name.value, reqInfo);
		flagTy->add_mention(name.range);
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, flagTy, mod)->id, ir::TypedType::get(ctx->irCtx));
	} else if (mod->has_region(name.value, reqInfo) || mod->has_brought_region(name.value, reqInfo) ||
	           mod->has_region_in_imports(name.value, reqInfo).first) {
		auto resRegion = mod->get_region(name.value, reqInfo);
		resRegion->add_mention(name.range);
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, resRegion, mod)->id,
		                            ir::TypedType::get(ctx->irCtx));
	} else if (mod->has_prerun_global(name.value, reqInfo) || mod->has_brought_prerun_global(name.value, reqInfo) ||
	           mod->has_prerun_global_in_imports(name.value, reqInfo).first) {
		auto resPre = mod->get_prerun_global(name.value, reqInfo);
		resPre->add_mention(name.range);
		return resPre;
	} else if (mod->has_global(name.value, reqInfo) || mod->has_brought_global(name.value, reqInfo) ||
	           mod->has_global_in_imports(name.value, reqInfo).first) {
		auto* gEnt = mod->get_global(name.value, reqInfo);
		gEnt->add_mention(name.range);
		if ((!gEnt->is_variable()) && gEnt->has_initial_value()) {
			return ir::PrerunValue::get(gEnt->get_initial_value(), gEnt->get_ir_type());
		} else {
			if (gEnt->is_variable()) {
				ctx->Error("Global entity " + ctx->color(gEnt->get_full_name()) +
				               " has variability, so it cannot be used in prerun expressions",
				           name.range);
			} else {
				ctx->Error("Global entity " + ctx->color(gEnt->get_full_name()) +
				               " doesn't have an initial value that is a prerun expressions",
				           name.range);
			}
		}
		ctx->Error(ctx->color(name.value) + " is a global entity.", name.range);
	} else if (mod->has_generic_struct_type(name.value, reqInfo) ||
	           mod->has_brought_generic_struct_type(name.value, reqInfo) ||
	           mod->has_generic_struct_type_in_imports(name.value, reqInfo).first) {
		ctx->Error(ctx->color(name.value) +
		               " is a generic core type and cannot be used as a value or type in prerun expressions",
		           name.range);
	} else if (mod->has_generic_type_def(name.value, reqInfo) ||
	           mod->has_brought_generic_type_def(name.value, reqInfo) ||
	           mod->has_generic_type_def_in_imports(name.value, reqInfo).first) {
		ctx->Error(ctx->color(name.value) +
		               " is a generic type definition and cannot be used as a value or type in prerun expressions",
		           name.range);
	} else if (mod->has_generic_function(name.value, reqInfo) ||
	           mod->has_brought_generic_function(name.value, reqInfo) ||
	           mod->has_generic_function_in_imports(name.value, reqInfo).first) {
		ctx->Error(ctx->color(name.value) +
		               " is a generic function and cannot be used as a value or type in prerun expressions",
		           fileRange);
	} else if (mod->has_prerun_function(name.value, reqInfo) || mod->has_brought_prerun_function(name.value, reqInfo) ||
	           mod->has_prerun_function_in_imports(name.value, reqInfo).first) {
		auto preFn = mod->get_prerun_function(name.value, reqInfo);
		preFn->add_mention(name.range);
		return preFn;
	}
	ctx->Error("No prerun entity named " + ctx->color(name.value) + " found", name.range);
	return nullptr;
}

Json PrerunEntity::to_json() const {
	Vec<JsonValue> idsJs;
	for (auto const& idnt : identifiers) {
		idsJs.push_back(idnt);
	}
	return Json()._("nodeType", "constantEntity")._("identifiers", idsJs)._("fileRange", fileRange);
}

String PrerunEntity::to_string() const {
	String result;
	for (usize i = 0; i < relative; i++) {
		result += "up:";
	}
	for (usize i = 0; i < identifiers.size(); i++) {
		result += identifiers.at(i).value;
		if (i != (identifiers.size() - 1)) {
			result += ":";
		}
	}
	return result;
}

} // namespace qat::ast
