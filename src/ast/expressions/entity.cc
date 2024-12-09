#include "./entity.hpp"
#include "../../IR/stdlib.hpp"
#include "../../IR/types/region.hpp"

#include <llvm/IR/GlobalVariable.h>
#include <utility>

namespace qat::ast {

void Entity::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) {
	auto mod     = ctx->mod;
	auto reqInfo = ctx->get_access_info();
	if ((relative != 0)) {
		if (ctx->mod->has_nth_parent(relative)) {
			mod = ctx->mod->get_nth_parent(relative);
		} else {
			ctx->Error("The current scope do not have " + (relative == 1 ? "a" : std::to_string(relative)) + " parent" +
			               (relative == 1 ? "" : "s") +
			               ". Relative mentions of identifiers cannot be used here. "
			               "Please check the logic.",
			           fileRange);
		}
	}
	if (names.size() > 1) {
		for (usize i = 0; i < (names.size() - 1); i++) {
			auto split = names.at(i);
			if (relative == 0 && i == 0 && split.value == "std" && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			} else if (relative == 0 && i == 0 && mod->has_entity_with_name(split.value)) {
				ent->addDependency(
				    ir::EntityDependency{mod->get_entity(split.value), dep.value_or(ir::DependType::complete), phase});
				break;
			}
			if (mod->has_lib(split.value, reqInfo)) {
				mod = mod->get_lib(split.value, reqInfo);
				mod->add_mention(split.range);
			} else if (mod->has_brought_mod(split.value, ctx->get_access_info()) ||
			           mod->has_brought_mod_in_imports(split.value, reqInfo).first) {
				mod = mod->get_brought_mod(split.value, reqInfo);
				mod->add_mention(split.range);
			} else {
				SHOW("Update deps")
				ctx->Error("No lib named " + ctx->color(split.value) + " found inside scope ", split.range);
			}
		}
	}
	auto entName = names.back();
	if (mod->has_entity_with_name(entName.value)) {
		ent->addDependency(
		    ir::EntityDependency{mod->get_entity(entName.value), dep.value_or(ir::DependType::complete), phase});
	}
}

ir::Value* Entity::emit(EmitCtx* ctx) {
	auto* fun     = ctx->get_fn();
	auto  reqInfo = ctx->get_access_info();
	auto* mod     = ctx->mod;
	if ((names.size() == 1) && (relative == 0)) {
		auto singleName = names.front();
		if (fun->has_generic_parameter(singleName.value)) {
			auto* genVal = fun->get_generic_parameter(singleName.value);
			if (genVal->is_typed()) {
				return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(genVal->as_typed()->get_type()));
			} else if (genVal->is_prerun()) {
				return genVal->as_prerun()->get_expression();
			} else {
				ctx->Error("Invalid generic kind", genVal->get_range());
			}
		} else if (ctx->has_member_parent() && ctx->get_member_parent()->is_expanded()) {
			auto actTy = ctx->get_member_parent()->as_expanded();
			if (actTy->is_expanded() && actTy->as_expanded()->has_generic_parameter(singleName.value)) {
				auto* genVal = actTy->as_expanded()->get_generic_parameter(singleName.value);
				if (genVal->is_typed()) {
					return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(genVal->as_typed()->get_type()));
				} else if (genVal->is_prerun()) {
					return genVal->as_prerun()->get_expression();
				} else {
					ctx->Error("Invalid generic kind", genVal->get_range());
				}
			}
		} else if (ctx->has_opaque_parent() && ctx->get_opaque_parent()->has_generic_parameter(singleName.value)) {
			auto genVal = ctx->get_opaque_parent()->get_generic_parameter(singleName.value);
			if (genVal->is_typed()) {
				return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(genVal->as_typed()->get_type()));
			} else if (genVal->is_prerun()) {
				return genVal->as_prerun()->get_expression();
			} else {
				ctx->Error("Invalid generic kind", genVal->get_range());
			}
		} else if (ctx->has_member_parent() && ctx->get_member_parent()->is_done_skill() &&
		           ctx->get_member_parent()->as_done_skill()->has_generic_parameter(singleName.value)) {
			auto* genVal = ctx->get_member_parent()->as_done_skill()->get_generic_parameter(singleName.value);
			if (genVal->is_typed()) {
				return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(genVal->as_typed()->get_type()));
			} else if (genVal->is_prerun()) {
				return genVal->as_prerun()->get_expression();
			} else {
				ctx->Error("Invalid generic kind", genVal->get_range());
			}
		}
		SHOW("Checking block " << fun->get_block()->get_name())
		if (fun->get_block()->has_value(singleName.value)) {
			SHOW("Found local value: " << singleName.value)
			auto* local = fun->get_block()->get_value(singleName.value);
			local->add_mention(singleName.range);
			auto* alloca = local->get_alloca();
			auto* val    = ir::Value::get(alloca, local->get_ir_type(), local->is_variable());
			SHOW("Returning local value with alloca name: " << alloca->getName().str())
			val->set_local_id(local->get_id());
			return val;
		} else {
			SHOW("No local value with name: " << singleName.value)
			// Checking arguments
			// Currently this is unnecessary, since all arguments are allocated as
			// local values first
			auto argTypes = fun->get_ir_type()->as_function()->get_argument_types();
			for (usize i = 0; i < argTypes.size(); i++) {
				if (fun->arg_name_at(i).value == singleName.value) {
					//   mod->addMention("parameter", singleName.range, fun->argumentNameAt(i).range);
					return ir::Value::get(llvm::dyn_cast<llvm::Function>(fun->get_llvm())->getArg(i),
					                      argTypes.at(i)->get_type(), false);
				}
			}
			// Checking functions
			if (mod->has_function(singleName.value, reqInfo) || mod->has_brought_function(singleName.value, reqInfo) ||
			    mod->has_function_in_imports(singleName.value, reqInfo).first) {
				return mod->get_function(singleName.value, reqInfo);
			} else if (mod->has_global(singleName.value, reqInfo) ||
			           mod->has_brought_global(singleName.value, reqInfo) ||
			           mod->has_global_in_imports(singleName.value, reqInfo).first) {
				auto* gEnt  = mod->get_global(singleName.value, reqInfo);
				auto  gName = llvm::cast<llvm::GlobalVariable>(gEnt->get_llvm())->getName();
				if (!ctx->mod->get_llvm_module()->getNamedGlobal(gName)) {
					new llvm::GlobalVariable(*ctx->mod->get_llvm_module(), gEnt->get_ir_type()->get_llvm_type(),
					                         !gEnt->is_variable(), llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage,
					                         nullptr, gName, nullptr,
					                         llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, std::nullopt, true);
				}
				return ir::Value::get(gEnt->get_llvm(), gEnt->get_ir_type(), gEnt->is_variable());
			}
		}
	} else {
		if ((relative != 0)) {
			if (ctx->mod->has_nth_parent(relative)) {
				mod = ctx->mod->get_nth_parent(relative);
			} else {
				ctx->Error("The current scope do not have " + (relative == 1 ? "a" : std::to_string(relative)) +
				               " parent" + (relative == 1 ? "" : "s") +
				               ". Relative mentions of identifiers cannot be used here. "
				               "Please check the logic.",
				           fileRange);
			}
		}
		if (names.size() > 1) {
			for (usize i = 0; i < (names.size() - 1); i++) {
				auto split = names.at(i);
				if (relative == 0 && i == 0 && split.value == "std" && ir::StdLib::is_std_lib_found()) {
					mod = ir::StdLib::stdLib;
					continue;
				}
				if (mod->has_lib(split.value, reqInfo)) {
					mod = mod->get_lib(split.value, reqInfo);
					mod->add_mention(split.range);
				} else if (mod->has_brought_mod(split.value, reqInfo) ||
				           mod->has_brought_mod_in_imports(split.value, reqInfo).first) {
					mod = mod->get_brought_mod(split.value, reqInfo);
					mod->add_mention(split.range);
				} else {
					SHOW("Emit fn")
					ctx->Error("No lib named " + ctx->color(split.value) + " found inside scope ", split.range);
				}
			}
		}
	}
	auto entityName = names.back();
	if (mod->has_function(entityName.value, reqInfo) || mod->has_brought_function(entityName.value, reqInfo) ||
	    mod->has_function_in_imports(entityName.value, reqInfo).first) {
		auto* fun = mod->get_function(entityName.value, reqInfo);
		if (!fun->is_accessible(reqInfo)) {
			ctx->Error("Function " + ctx->color(fun->get_full_name()) + " is not accessible here", fileRange);
		}
		fun->add_mention(entityName.range);
		return fun;
	} else if (mod->has_global(entityName.value, reqInfo) || mod->has_brought_global(entityName.value, reqInfo) ||
	           mod->has_global_in_imports(entityName.value, reqInfo).first) {
		auto* gEnt  = mod->get_global(entityName.value, reqInfo);
		auto  gName = llvm::cast<llvm::GlobalVariable>(gEnt->get_llvm())->getName();
		if (!ctx->mod->get_llvm_module()->getNamedGlobal(gName)) {
			new llvm::GlobalVariable(*ctx->mod->get_llvm_module(), gEnt->get_ir_type()->get_llvm_type(),
			                         !gEnt->is_variable(), llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage,
			                         nullptr, gName, nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal,
			                         std::nullopt, true);
		}
		if (!gEnt->get_visibility().is_accessible(reqInfo)) {
			ctx->Error("Global entity " + ctx->color(gEnt->get_full_name()) + " is not accessible here", fileRange);
		}
		gEnt->add_mention(entityName.range);
		return ir::Value::get(gEnt->get_llvm(), gEnt->get_ir_type(), gEnt->is_variable());
	} else {
		if (mod->has_lib(entityName.value, reqInfo) || mod->has_brought_lib(entityName.value, reqInfo) ||
		    mod->has_lib_in_imports(entityName.value, reqInfo).first) {
			ctx->Error(mod->get_lib(entityName.value, reqInfo)->get_full_name() +
			               " is a lib and cannot be used as a value in an expression",
			           entityName.range);
		} else if (mod->has_struct_type(entityName.value, reqInfo) ||
		           mod->has_brought_struct_type(entityName.value, reqInfo) ||
		           mod->has_struct_type_in_imports(entityName.value, reqInfo).first) {
			auto resCoreType = mod->get_struct_type(entityName.value, reqInfo);
			resCoreType->add_mention(entityName.range);
			return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(resCoreType));
		} else if (mod->has_mix_type(entityName.value, reqInfo) ||
		           mod->has_brought_mix_type(entityName.value, reqInfo) ||
		           mod->has_mix_type_in_imports(entityName.value, reqInfo).first) {
			auto resMixType = mod->get_mix_type(entityName.value, reqInfo);
			resMixType->add_mention(entityName.range);
			return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(resMixType));
		} else if (mod->has_choice_type(entityName.value, reqInfo) ||
		           mod->has_brought_choice_type(entityName.value, reqInfo) ||
		           mod->has_choice_type_in_imports(entityName.value, reqInfo).first) {
			auto* resChoiceTy = mod->get_choice_type(entityName.value, reqInfo);
			resChoiceTy->add_mention(entityName.range);
			return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(resChoiceTy));
		} else if (mod->has_region(entityName.value, reqInfo) || mod->has_brought_region(entityName.value, reqInfo) ||
		           mod->has_region_in_imports(entityName.value, reqInfo).first) {
			auto* resRegion = mod->get_region(entityName.value, reqInfo);
			resRegion->add_mention(entityName.range);
			return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(resRegion));
		} else if (mod->has_prerun_global(entityName.value, reqInfo) ||
		           mod->has_brought_prerun_global(entityName.value, reqInfo) ||
		           mod->has_prerun_global_in_imports(entityName.value, reqInfo).first) {
			auto* resPre = mod->get_prerun_global(entityName.value, reqInfo);
			resPre->add_mention(entityName.range);
			return resPre;
		} else if (mod->has_type_definition(entityName.value, reqInfo) ||
		           mod->has_brought_type_definition(entityName.value, reqInfo) ||
		           mod->has_type_definition_in_imports(entityName.value, reqInfo).first) {
			auto* resTy = mod->get_type_def(entityName.value, reqInfo);
			resTy->add_mention(entityName.range);
			return ir::PrerunValue::get_typed_prerun(ir::TypedType::get(resTy));
		} else if (mod->has_prerun_function(entityName.value, reqInfo) ||
		           mod->has_brought_prerun_function(entityName.value, reqInfo) ||
		           mod->has_prerun_function_in_imports(entityName.value, reqInfo).first) {
			auto* preFn = mod->get_prerun_function(entityName.value, reqInfo);
			preFn->add_mention(entityName.range);
			return preFn;
		} else if (mod->has_generic_struct_type(entityName.value, reqInfo) ||
		           mod->has_brought_generic_struct_type(entityName.value, reqInfo) ||
		           mod->has_generic_struct_type_in_imports(entityName.value, reqInfo).first) {
			ctx->Error(ctx->color(entityName.value) + " is a generic core type and cannot be used as a value or type",
			           entityName.range);
		} else if (mod->has_generic_type_def(entityName.value, reqInfo) ||
		           mod->has_brought_generic_type_def(entityName.value, reqInfo) ||
		           mod->has_generic_type_def_in_imports(entityName.value, reqInfo).first) {
			ctx->Error(ctx->color(entityName.value) +
			               " is a generic type definition and cannot be used as a value or type",
			           entityName.range);
		} else if (mod->has_generic_function(entityName.value, reqInfo) ||
		           mod->has_brought_generic_function(entityName.value, reqInfo) ||
		           mod->has_generic_function_in_imports(entityName.value, reqInfo).first) {
			ctx->Error(ctx->color(entityName.value) + " is a generic function and cannot be used as a value or type",
			           fileRange);
		} else {
			ctx->Error("No entity named " + ctx->color(Identifier::fullName(names).value) + " found", fileRange);
		}
	}
	return nullptr;
}

Json Entity::to_json() const {
	Vec<JsonValue> namesJs;
	for (auto const& nam : names) {
		namesJs.push_back(JsonValue(nam));
	}
	return Json()._("nodeType", "entity")._("names", namesJs)._("relative", relative)._("fileRange", fileRange);
}

} // namespace qat::ast
