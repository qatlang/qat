#include "./bring_entities.hpp"
#include "../IR/stdlib.hpp"
#include "../IR/types/region.hpp"
#include "emit_ctx.hpp"
#include <utility>

namespace qat::ast {

void BroughtGroup::addMember(BroughtGroup* mem) { members.push_back(mem); }

void BroughtGroup::extendFileRange(FileRange end) { fileRange = FileRange(fileRange, end); }

bool BroughtGroup::hasMembers() const { return !members.empty(); }

bool BroughtGroup::isAllBrought() const { return members.empty(); }

void BroughtGroup::bring() const {
	isAlreadyBrought = true;
	if (entityState) {
		entityState->updateStatus(ir::EntityStatus::complete);
	}
}

Json BroughtGroup::to_json() const {
	Vec<JsonValue> entityName;
	for (auto const& idn : entity) {
		entityName.push_back(Json()._("value", idn.value)._("range", idn.range));
	}
	Vec<JsonValue> membersJson;
	for (auto const& mem : members) {
		membersJson.push_back(mem->to_json());
	}
	return Json()._("relative", relative)._("entity", entityName)._("members", membersJson)._("fileRange", fileRange);
}

void BringEntities::create_entity(ir::Mod* currMod, ir::Ctx* irCtx) {
	auto ctx	= EmitCtx::get(irCtx, currMod);
	entityState = currMod->add_entity(None, ir::EntityType::bringEntity, this, ir::EmitPhase::phase_1);
	auto										 reqInfo	   = ctx->get_access_info();
	std::function<void(BroughtGroup*, ir::Mod*)> createHandler = [&](BroughtGroup* ent, ir::Mod* parentMod) {
		auto mod = parentMod;
		if (ent->relative > 0) {
			if (parentMod->has_nth_parent(ent->relative)) {
				mod = parentMod->get_nth_parent(ent->relative);
			} else {
				ctx->Error("Current module does not have " + std::to_string(ent->relative) + " parents",
						   ent->fileRange);
			}
		}
		for (usize i = 0; i < (ent->entity.size() - 1); i++) {
			auto const& idn = ent->entity.at(i);
			if ((ent->relative == 0) && (i == 0) && (idn.value == "std") && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				mod->add_mention(idn.range);
				continue;
			}
			if (mod->has_lib(idn.value, reqInfo) || mod->has_brought_lib(idn.value, reqInfo) ||
				mod->has_lib_in_imports(idn.value, reqInfo).first) {
				mod = mod->get_lib(idn.value, reqInfo);
				mod->add_mention(idn.range);
				if (!mod->get_visibility().is_accessible(reqInfo)) {
					ctx->Error("This lib is not accessible in the current scope", idn.range);
				}
			} else if (mod->has_brought_mod(idn.value, reqInfo)) {
				mod = mod->get_brought_mod(idn.value, reqInfo);
				mod->add_mention(idn.range);
				if (!mod->get_visibility().is_accessible(reqInfo)) {
					ctx->Error("This brought module is not accessible in the current scope", idn.range);
				}
			} else {
				ctx->Error("No lib or brought module named " + ctx->color(idn.value) + " found", idn.range);
			}
		}
		auto entName = ent->entity.back();
		// SHOW("BringEntities name " << entName.value)
		if (mod->has_lib(entName.value, reqInfo) || mod->has_brought_lib(entName.value, reqInfo) ||
			mod->has_lib_in_imports(entName.value, reqInfo).first) {
			SHOW("BringEntities: name " << entName.value)
			mod = mod->get_lib(entName.value, reqInfo);
			if (!mod->get_visibility().is_accessible(reqInfo)) {
				ctx->Error("Lib " + ctx->color(entName.value) + " is not accessible in the current scope",
						   entName.range);
			}
			if (ent->isAllBrought()) {
				ent->entityState =
					currMod->add_entity(entName, ir::EntityType::bringEntity, this, ir::EmitPhase::phase_1);
				entityState->addDependency(
					ir::EntityDependency{ent->entityState, ir::DependType::complete, ir::EmitPhase::phase_1});
				currMod->bring_module(mod, ctx->get_visibility_info(visibSpec));
				mod->add_bring_mention(currMod, ent->fileRange);
				ent->bring();
			} else {
				for (auto& mem : ent->members) {
					createHandler(mem, mod);
				}
			}
		} else if (mod->has_brought_mod(entName.value, reqInfo)) {
			mod = mod->get_brought_mod(entName.value, reqInfo);
			if (!mod->get_visibility().is_accessible(reqInfo)) {
				ctx->Error("Brought module " + ctx->color(entName.value) + " is not accessible in the current scope",
						   entName.range);
			}
			if (ent->isAllBrought()) {
				ent->entityState =
					currMod->add_entity(entName, ir::EntityType::bringEntity, this, ir::EmitPhase::phase_1);
				entityState->addDependency(
					ir::EntityDependency{ent->entityState, ir::DependType::complete, ir::EmitPhase::phase_1});
				currMod->bring_module(mod, ctx->get_visibility_info(visibSpec));
				mod->add_bring_mention(currMod, ent->fileRange);
				ent->bring();
			} else {
				for (auto& mem : ent->members) {
					createHandler(mem, mod);
				}
			}
		} else {
			if (ent->hasMembers()) {
				ctx->Error(ctx->color(entName.value) + " is not a module and hence you cannot bring its members",
						   entName.range);
			}
			ent->entityState = currMod->add_entity(entName, ir::EntityType::bringEntity, this, ir::EmitPhase::phase_1);
		}
	};
	for (auto entity : entities) {
		createHandler(entity, currMod);
	}
}

void BringEntities::update_entity_dependencies(ir::Mod* currMod, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, currMod);
	auto reqInfo = emitCtx->get_access_info();

	std::function<void(BroughtGroup*, ir::Mod*)> updateHandler = [&](BroughtGroup* ent, ir::Mod* parentMod) {
		auto mod = parentMod;
		if (ent->relative > 0) {
			if (parentMod->has_nth_parent(ent->relative)) {
				mod = parentMod->get_nth_parent(ent->relative);
			} else {
				irCtx->Error("Current module does not have " + std::to_string(ent->relative) + " parents",
							 ent->fileRange);
			}
		}
		for (usize i = 0; i < (ent->entity.size() - 1); i++) {
			auto const& idn = ent->entity.at(i);
			if ((ent->relative == 0) && (i == 0) && (idn.value == "std") && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				mod->add_mention(idn.range);
				continue;
			}
			if (mod->has_lib(idn.value, reqInfo) || mod->has_brought_lib(idn.value, reqInfo) ||
				mod->has_lib_in_imports(idn.value, reqInfo).first) {
				mod = mod->get_lib(idn.value, reqInfo);
				if (!mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This lib is not accessible in the current scope", idn.range);
				}
			} else if (mod->has_brought_mod(idn.value, reqInfo)) {
				mod = mod->get_brought_mod(idn.value, reqInfo);
				if (!mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This brought module is not accessible in the current scope", idn.range);
				}
			} else {
				irCtx->Error("No lib or brought module named " + irCtx->color(idn.value) + " found", idn.range);
			}
		}
		auto entName = ent->entity.back();
		if (mod->has_lib(entName.value, reqInfo) || mod->has_brought_lib(entName.value, reqInfo) ||
			mod->has_lib_in_imports(entName.value, reqInfo).first) {
			mod = mod->get_lib(entName.value, reqInfo);
			if (!mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Lib " + irCtx->color(entName.value) + " is not accessible in the current scope",
							 entName.range);
			}
			if (!ent->isAllBrought()) {
				for (auto& mem : ent->members) {
					updateHandler(mem, mod);
				}
			}
		} else if (mod->has_brought_mod(entName.value, reqInfo)) {
			mod = mod->get_brought_mod(entName.value, reqInfo);
			if (!mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Brought module " + irCtx->color(entName.value) +
								 " is not accessible in the current scope",
							 entName.range);
			}
			if (!ent->isAllBrought()) {
				for (auto& mem : ent->members) {
					updateHandler(mem, mod);
				}
			}
		} else {
			if (ent->hasMembers()) {
				irCtx->Error(irCtx->color(entName.value) + " is not a module and hence you cannot bring its members",
							 entName.range);
			}
			if (mod->has_entity_with_name(entName.value)) {
				ent->entityState->addDependency(ir::EntityDependency{mod->get_entity(entName.value),
																	 ir::DependType::complete, ir::EmitPhase::phase_1});
			} else {
				bool						  foundIt	 = false;
				std::function<bool(ir::Mod*)> modHandler = [&](ir::Mod* module) -> bool {
					for (auto sub : module->submodules) {
						if (!sub->should_be_named() && sub->has_entity_with_name(entName.value)) {
							ent->entityState->addDependency(ir::EntityDependency{
								sub->get_entity(entName.value), ir::DependType::complete, ir::EmitPhase::phase_1});
							return true;
						} else if (!sub->should_be_named()) {
							if (modHandler(sub)) {
								return true;
							}
						}
					}
					for (auto bMod : module->broughtModules) {
						if (!bMod.is_named() && bMod.get()->has_entity_with_name(entName.value)) {
							ent->entityState->addDependency(ir::EntityDependency{bMod.get()->get_entity(entName.value),
																				 ir::DependType::complete,
																				 ir::EmitPhase::phase_1});
							return true;
						} else if (!bMod.is_named()) {
							if (modHandler(bMod.get())) {
								return true;
							}
						}
					}
					return false;
				};
				auto modRes = modHandler(mod);
				if (!modRes) {
					irCtx->Error("No recognisable entity named " + irCtx->color(entName.value) +
									 " could be found in the provided parent module " + irCtx->color(mod->get_name()) +
									 " in file " + mod->get_file_path(),
								 entName.range);
				}
			}
		}
	};
	for (auto ent : entities) {
		updateHandler(ent, currMod);
	}
}

void BringEntities::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) { handle_brings(mod, irCtx); }

void BringEntities::handle_brings(ir::Mod* currentMod, ir::Ctx* irCtx) const {
	auto emitCtx = EmitCtx::get(irCtx, currentMod);
	auto reqInfo = emitCtx->get_access_info();

	std::function<void(BroughtGroup*, ir::Mod*)> bringHandler = [&](BroughtGroup* ent, ir::Mod* parentMod) {
		ir::Mod* mod = parentMod;
		if (ent->isAlreadyBrought) {
			return;
		}
		if (ent->relative > 0) {
			if (parentMod->has_nth_parent(ent->relative)) {
				mod = parentMod->get_nth_parent(ent->relative);
			} else {
				irCtx->Error("Current module does not have " + std::to_string(ent->relative) + " parents",
							 ent->fileRange);
			}
		}
		for (usize i = 0; i < (ent->entity.size() - 1); i++) {
			auto const& idn = ent->entity.at(i);
			if ((ent->relative == 0) && (i == 0) && (idn.value == "std") && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			}
			if (mod->has_lib(idn.value, reqInfo) || mod->has_brought_lib(idn.value, reqInfo) ||
				mod->has_lib_in_imports(idn.value, reqInfo).first) {
				mod = mod->get_lib(idn.value, reqInfo);
				if (!mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This lib is not accessible in the current scope", idn.range);
				}
			} else if (mod->has_brought_mod(idn.value, reqInfo)) {
				mod = mod->get_brought_mod(idn.value, reqInfo);
				if (!mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This brought module is not accessible in the current scope", idn.range);
				}
			} else {
				irCtx->Error("No lib or brought module named " + irCtx->color(idn.value) + " found", idn.range);
			}
		}
		auto entName = ent->entity.back();
		if (mod->has_lib(entName.value, reqInfo) || mod->has_brought_lib(entName.value, reqInfo) ||
			mod->has_lib_in_imports(entName.value, reqInfo).first) {
			mod = mod->get_lib(entName.value, reqInfo);
			if (!mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Lib " + irCtx->color(entName.value) + " is not accessible in the current scope",
							 entName.range);
			}
			if (ent->isAllBrought()) {
				currentMod->bring_module(mod, emitCtx->get_visibility_info(visibSpec));
				mod->add_bring_mention(currentMod, ent->fileRange);
				ent->bring();
			} else {
				for (auto& mem : ent->members) {
					bringHandler(mem, mod);
				}
			}
		} else if (mod->has_brought_mod(entName.value, reqInfo)) {
			mod = mod->get_brought_mod(entName.value, reqInfo);
			if (!mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Brought module " + irCtx->color(entName.value) +
								 " is not accessible in the current scope",
							 entName.range);
			}
			if (ent->isAllBrought()) {
				currentMod->bring_module(mod, emitCtx->get_visibility_info(visibSpec));
				mod->add_bring_mention(currentMod, ent->fileRange);
				ent->bring();
			} else {
				for (auto& mem : ent->members) {
					bringHandler(mem, mod);
				}
			}
		} else {
			if (ent->hasMembers()) {
				irCtx->Error(irCtx->color(entName.value) + " is not a module and hence you cannot bring its members",
							 entName.range);
			}
			if (mod->has_opaque_type(entName.value, reqInfo) || mod->has_brought_opaque_type(entName.value, reqInfo) ||
				mod->has_opaque_type_in_imports(entName.value, reqInfo).first) {
				auto* oTy = mod->get_opaque_type(entName.value, reqInfo);
				if (!oTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Opaque type " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_opaque_type(oTy, emitCtx->get_visibility_info(visibSpec));
				oTy->add_bring_mention(currentMod, ent->fileRange);
				ent->bring();
			} else if (mod->has_struct_type(entName.value, reqInfo) ||
					   mod->has_brought_struct_type(entName.value, reqInfo) ||
					   mod->has_struct_type_in_imports(entName.value, reqInfo).first) {
				SHOW("Bring entity is struct")
				auto* cTy = mod->get_struct_type(entName.value, reqInfo);
				if (!cTy->is_accessible(reqInfo)) {
					irCtx->Error("Core type " + irCtx->color(entName.value) + " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_struct_type(cTy, emitCtx->get_visibility_info(visibSpec));
				cTy->add_bring_mention(currentMod, ent->fileRange);
				ent->bring();
			} else if (mod->has_choice_type(entName.value, reqInfo) ||
					   mod->has_brought_choice_type(entName.value, reqInfo) ||
					   mod->has_choice_type_in_imports(entName.value, reqInfo).first) {
				auto* chTy = mod->get_choice_type(entName.value, reqInfo);
				if (!chTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Choice type " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_choice_type(chTy, emitCtx->get_visibility_info(visibSpec));
				chTy->add_bring_mention(currentMod, ent->fileRange);
				ent->bring();
			} else if (mod->has_mix_type(entName.value, reqInfo) || mod->has_brought_mix_type(entName.value, reqInfo) ||
					   mod->has_mix_type_in_imports(entName.value, reqInfo).first) {
				auto* mTy = mod->get_mix_type(entName.value, reqInfo);
				if (!mTy->is_accessible(reqInfo)) {
					irCtx->Error("Mix type " + irCtx->color(entName.value) + " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_mix_type(mTy, emitCtx->get_visibility_info(visibSpec));
				mTy->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_type_definition(entName.value, reqInfo) ||
					   mod->has_brought_type_definition(entName.value, reqInfo) ||
					   mod->has_type_definition_in_imports(entName.value, reqInfo).first) {
				auto* dTy = mod->get_type_def(entName.value, reqInfo);
				if (!dTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Type definition " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_type_definition(dTy, emitCtx->get_visibility_info(visibSpec));
				dTy->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_region(entName.value, reqInfo) || mod->has_brought_region(entName.value, reqInfo) ||
					   mod->has_region_in_imports(entName.value, reqInfo).first) {
				auto* rTy = mod->get_region(entName.value, reqInfo);
				if (!rTy->is_accessible(reqInfo)) {
					irCtx->Error("Region " + irCtx->color(entName.value) + " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_region(rTy, emitCtx->get_visibility_info(visibSpec));
				rTy->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_function(entName.value, reqInfo) || mod->has_brought_function(entName.value, reqInfo) ||
					   mod->has_function_in_imports(entName.value, reqInfo).first) {
				auto* otherFn = mod->get_function(entName.value, reqInfo);
				if (!otherFn->is_accessible(reqInfo)) {
					irCtx->Error("Function " + irCtx->color(entName.value) + " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_function(otherFn, emitCtx->get_visibility_info(visibSpec));
				otherFn->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_generic_function(entName.value, reqInfo) ||
					   mod->has_brought_generic_function(entName.value, reqInfo) ||
					   mod->has_generic_function_in_imports(entName.value, reqInfo).first) {
				auto* gnFn = mod->get_generic_function(entName.value, reqInfo);
				if (!gnFn->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Generic function " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_generic_function(gnFn, emitCtx->get_visibility_info(visibSpec));
				gnFn->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_generic_struct_type(entName.value, reqInfo) ||
					   mod->has_brought_generic_struct_type(entName.value, reqInfo) ||
					   mod->has_generic_struct_type_in_imports(entName.value, reqInfo).first) {
				auto* gnCTy = mod->get_generic_struct_type(entName.value, reqInfo);
				if (!gnCTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Generic core type " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_generic_struct_type(gnCTy, emitCtx->get_visibility_info(visibSpec));
				gnCTy->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_global(entName.value, reqInfo) || mod->has_brought_global(entName.value, reqInfo) ||
					   mod->has_global_in_imports(entName.value, reqInfo).first) {
				auto* gEnt = mod->get_global(entName.value, reqInfo);
				if (!gEnt->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Global entity " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_global(gEnt, emitCtx->get_visibility_info(visibSpec));
				gEnt->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_prerun_global(entName.value, reqInfo) ||
					   mod->has_brought_prerun_global(entName.value, reqInfo) ||
					   mod->has_prerun_global_in_imports(entName.value, reqInfo).first) {
				auto* gEnt = mod->get_prerun_global(entName.value, reqInfo);
				if (!gEnt->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Prerun global entity " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_prerun_global(gEnt, emitCtx->get_visibility_info(visibSpec));
				gEnt->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (mod->has_prerun_function(entName.value, reqInfo) ||
					   mod->has_brought_prerun_function(entName.value, reqInfo) ||
					   mod->has_prerun_function_in_imports(entName.value, reqInfo).first) {
				auto* preFn = mod->get_prerun_function(entName.value, reqInfo);
				if (not preFn->get_visibility().is_accessible(
						reqInfo)) { // TODO - Verify that this check is necessary. It most probably isn't. If not remove
									// this and the above checks as well
					irCtx->Error("Prerun function " + irCtx->color(entName.value) +
									 " is not accessible in the current scope",
								 entName.range);
				}
				currentMod->bring_prerun_function(preFn, emitCtx->get_visibility_info(visibSpec));
				preFn->add_bring_mention(currentMod, entName.range);
				ent->bring();
			} else if (throwErrorsWhenUnfound) {
				irCtx->Error("No module, type, function, region , prerun global, or global named " +
								 irCtx->color(entName.value) + " found in the parent scope",
							 entName.range);
			}
		}
	};
	for (auto ent : entities) {
		bringHandler(ent, currentMod);
	}
	// FIXME - Order of declaration can cause issues
}

Json BringEntities::to_json() const {
	Vec<JsonValue> entitiesJson;
	for (auto const& ent : entities) {
		entitiesJson.emplace_back(ent->to_json());
	}
	return Json()
		._("nodeType", "bringEntities")
		._("entities", entitiesJson)
		._("hasVisibility", visibSpec.has_value())
		._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
		._("fileRange", fileRange);
}

BringEntities::~BringEntities() {
	for (auto* ent : entities) {
		std::destroy_at(ent);
	}
}

} // namespace qat::ast
