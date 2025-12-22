#include "./import_entities.hpp"
#include "../IR/stdlib.hpp"
#include "../IR/types/region.hpp"
#include "../IR/types/struct_type.hpp"
#include "../IR/types/toggle.hpp"
#include "../utils/constants.hpp"
#include "./emit_ctx.hpp"

namespace qat::ast {

void ImportGroup::add_member(ImportGroup* mem) { members.push_back(mem); }

void ImportGroup::extend_filerange(FileRangePtr end) { fileRange = FileRange::merge(fileRange, end); }

bool ImportGroup::has_members() const { return not members.empty(); }

bool ImportGroup::is_all_imported() const { return members.empty(); }

void ImportGroup::perform_import() const {
	isAlreadyImported = true;
	if (entityState) {
		entityState->updateStatus(ir::EntityStatus::complete);
	}
}

Json ImportGroup::to_json() const {
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

void ImportEntities::create_entity(ir::Mod* currMod, ir::Ctx* irCtx) {
	auto ctx    = EmitCtx::get(irCtx, currMod);
	entityState = currMod->add_entity(None, ir::EntityType::importEntity, this, ir::EmitPhase::phase_1);
	auto                                        reqInfo       = ctx->get_access_info();
	std::function<void(ImportGroup*, ir::Mod*)> createHandler = [&](ImportGroup* ent, ir::Mod* parentMod) {
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
			if ((ent->relative == 0) && (i == 0) && (idn.value == CORELIB) && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				mod->add_mention(idn.range);
				continue;
			}
			if (mod->has_lib(idn.value, reqInfo) || mod->has_imported_lib(idn.value, reqInfo) ||
			    mod->has_lib_in_imports(idn.value, reqInfo).first) {
				mod = mod->get_lib(idn.value, reqInfo);
				mod->add_mention(idn.range);
				if (not mod->get_visibility().is_accessible(reqInfo)) {
					ctx->Error("This lib is not accessible in the current scope", idn.range);
				}
			} else if (mod->has_imported_mod(idn.value, reqInfo)) {
				mod = mod->get_imported_mod(idn.value, reqInfo);
				mod->add_mention(idn.range);
				if (not mod->get_visibility().is_accessible(reqInfo)) {
					ctx->Error("This imported module is not accessible in the current scope", idn.range);
				}
			} else {
				ctx->Error("No lib or imported module named " + ctx->color(idn.value) + " found", idn.range);
			}
		}
		auto entName   = ent->entity.back();
		auto nameInMod = ent->alias.has_value() ? ent->alias : ent->entity.back();
		// SHOW("ImportEntities name " << entName.value)
		if (mod->has_lib(entName.value, reqInfo) || mod->has_imported_lib(entName.value, reqInfo) ||
		    mod->has_lib_in_imports(entName.value, reqInfo).first) {
			SHOW("ImportEntities: name " << entName.value)
			mod = mod->get_lib(entName.value, reqInfo);
			if (not mod->get_visibility().is_accessible(reqInfo)) {
				ctx->Error("Lib " + ctx->color(entName.value) + " is not accessible in the current scope",
				           entName.range);
			}
			if (ent->is_all_imported()) {
				ent->entityState =
				    currMod->add_entity(nameInMod, ir::EntityType::importEntity, this, ir::EmitPhase::phase_1);
				entityState->addDependency(
				    ir::EntityDependency{ent->entityState, ir::DependType::complete, ir::EmitPhase::phase_1});
				currMod->import_module(mod, ctx->get_visibility_info(visibSpec), ent->alias);
				mod->add_import_mention(currMod, ent->entity.back().range);
				ent->perform_import();
			} else {
				for (auto& mem : ent->members) {
					createHandler(mem, mod);
				}
			}
		} else if (mod->has_imported_mod(entName.value, reqInfo)) {
			mod = mod->get_imported_mod(entName.value, reqInfo);
			if (not mod->get_visibility().is_accessible(reqInfo)) {
				ctx->Error("Imported module " + ctx->color(entName.value) + " is not accessible in the current scope",
				           entName.range);
			}
			if (ent->is_all_imported()) {
				ent->entityState =
				    currMod->add_entity(nameInMod, ir::EntityType::importEntity, this, ir::EmitPhase::phase_1);
				entityState->addDependency(
				    ir::EntityDependency{ent->entityState, ir::DependType::complete, ir::EmitPhase::phase_1});
				currMod->import_module(mod, ctx->get_visibility_info(visibSpec), ent->alias);
				mod->add_import_mention(currMod, ent->entity.back().range);
				ent->perform_import();
			} else {
				for (auto& mem : ent->members) {
					createHandler(mem, mod);
				}
			}
		} else {
			if (ent->has_members()) {
				ctx->Error(ctx->color(entName.value) + " is not a module and hence you cannot imoprt its members",
				           entName.range);
			}
			ent->entityState =
			    currMod->add_entity(nameInMod, ir::EntityType::importEntity, this, ir::EmitPhase::phase_1);
		}
	};
	for (auto entity : entities) {
		createHandler(entity, currMod);
	}
}

void ImportEntities::update_entity_dependencies(ir::Mod* currMod, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, currMod);
	auto reqInfo = emitCtx->get_access_info();

	std::function<void(ImportGroup*, ir::Mod*)> updateHandler = [&](ImportGroup* ent, ir::Mod* parentMod) {
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
			if ((ent->relative == 0) && (i == 0) && (idn.value == CORELIB) && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				mod->add_mention(idn.range);
				continue;
			}
			if (mod->has_lib(idn.value, reqInfo) || mod->has_imported_lib(idn.value, reqInfo) ||
			    mod->has_lib_in_imports(idn.value, reqInfo).first) {
				mod = mod->get_lib(idn.value, reqInfo);
				if (not mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This lib is not accessible in the current scope", idn.range);
				}
			} else if (mod->has_imported_mod(idn.value, reqInfo)) {
				mod = mod->get_imported_mod(idn.value, reqInfo);
				if (not mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This imported module is not accessible in the current scope", idn.range);
				}
			} else {
				irCtx->Error("No lib or imported module named " + irCtx->color(idn.value) + " found", idn.range);
			}
		}
		auto entName = ent->entity.back();
		if (mod->has_lib(entName.value, reqInfo) || mod->has_imported_lib(entName.value, reqInfo) ||
		    mod->has_lib_in_imports(entName.value, reqInfo).first) {
			mod = mod->get_lib(entName.value, reqInfo);
			if (not mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Lib " + irCtx->color(entName.value) + " is not accessible in the current scope",
				             entName.range);
			}
			if (not ent->is_all_imported()) {
				for (auto& mem : ent->members) {
					updateHandler(mem, mod);
				}
			}
		} else if (mod->has_imported_mod(entName.value, reqInfo)) {
			mod = mod->get_imported_mod(entName.value, reqInfo);
			if (not mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Imported module " + irCtx->color(entName.value) +
				                 " is not accessible in the current scope",
				             entName.range);
			}
			if (not ent->is_all_imported()) {
				for (auto& mem : ent->members) {
					updateHandler(mem, mod);
				}
			}
		} else {
			if (ent->has_members()) {
				irCtx->Error(irCtx->color(entName.value) + " is not a module and hence you cannot import its members",
				             entName.range);
			}
			if (mod->has_entity_with_name(entName.value)) {
				ent->entityState->addDependency(ir::EntityDependency{mod->get_entity(entName.value),
				                                                     ir::DependType::complete, ir::EmitPhase::phase_1});
			} else {
				std::function<bool(ir::Mod*)> modHandler = [&](ir::Mod* module) -> bool {
					for (auto sub : module->submodules) {
						if ((not sub->should_be_named()) && sub->has_entity_with_name(entName.value)) {
							ent->entityState->addDependency(ir::EntityDependency{
							    sub->get_entity(entName.value), ir::DependType::complete, ir::EmitPhase::phase_1});
							return true;
						} else if (not sub->should_be_named()) {
							if (modHandler(sub)) {
								return true;
							}
						}
					}
					for (auto bMod : module->importedModules) {
						if ((not bMod.is_named()) && bMod.get()->has_entity_with_name(entName.value)) {
							ent->entityState->addDependency(ir::EntityDependency{bMod.get()->get_entity(entName.value),
							                                                     ir::DependType::complete,
							                                                     ir::EmitPhase::phase_1});
							return true;
						} else if (not bMod.is_named()) {
							if (modHandler(bMod.get())) {
								return true;
							}
						}
					}
					return false;
				};
				auto modRes = modHandler(mod);
				if (not modRes) {
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

void ImportEntities::do_phase(ir::EmitPhase, ir::Mod* mod, ir::Ctx* irCtx) { handle_imports(mod, irCtx); }

void ImportEntities::handle_imports(ir::Mod* currentMod, ir::Ctx* irCtx) const {
	auto emitCtx = EmitCtx::get(irCtx, currentMod);
	auto reqInfo = emitCtx->get_access_info();

	std::function<void(ImportGroup*, ir::Mod*)> importHandler = [&](ImportGroup* ent, ir::Mod* parentMod) {
		ir::Mod* mod = parentMod;
		if (ent->isAlreadyImported) {
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
			if ((ent->relative == 0) && (i == 0) && (idn.value == CORELIB) && ir::StdLib::is_std_lib_found()) {
				mod = ir::StdLib::stdLib;
				continue;
			}
			if (mod->has_lib(idn.value, reqInfo) || mod->has_imported_lib(idn.value, reqInfo) ||
			    mod->has_lib_in_imports(idn.value, reqInfo).first) {
				mod = mod->get_lib(idn.value, reqInfo);
				if (not mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This lib is not accessible in the current scope", idn.range);
				}
			} else if (mod->has_imported_mod(idn.value, reqInfo)) {
				mod = mod->get_imported_mod(idn.value, reqInfo);
				if (not mod->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("This imported module is not accessible in the current scope", idn.range);
				}
			} else {
				irCtx->Error("No lib or imported module named " + irCtx->color(idn.value) + " found", idn.range);
			}
		}
		auto entName = ent->entity.back();
		if (mod->has_lib(entName.value, reqInfo) || mod->has_imported_lib(entName.value, reqInfo) ||
		    mod->has_lib_in_imports(entName.value, reqInfo).first) {
			mod = mod->get_lib(entName.value, reqInfo);
			if (not mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Lib " + irCtx->color(entName.value) + " is not accessible in the current scope",
				             entName.range);
			}
			if (ent->is_all_imported()) {
				currentMod->import_module(mod, emitCtx->get_visibility_info(visibSpec), ent->alias);
				mod->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else {
				for (auto& mem : ent->members) {
					importHandler(mem, mod);
				}
			}
		} else if (mod->has_imported_mod(entName.value, reqInfo)) {
			mod = mod->get_imported_mod(entName.value, reqInfo);
			if (not mod->get_visibility().is_accessible(reqInfo)) {
				irCtx->Error("Imported module " + irCtx->color(entName.value) +
				                 " is not accessible in the current scope",
				             entName.range);
			}
			if (ent->is_all_imported()) {
				currentMod->import_module(mod, emitCtx->get_visibility_info(visibSpec), ent->alias);
				mod->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else {
				for (auto& mem : ent->members) {
					importHandler(mem, mod);
				}
			}
		} else {
			if (ent->has_members()) {
				irCtx->Error(irCtx->color(entName.value) + " is not a module and hence you cannot import its members",
				             entName.range);
			}
			if (mod->has_opaque_type(entName.value, reqInfo) || mod->has_imported_opaque_type(entName.value, reqInfo) ||
			    mod->has_opaque_type_in_imports(entName.value, reqInfo).first) {
				auto* oTy = mod->get_opaque_type(entName.value, reqInfo);
				if (not oTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Opaque type " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_opaque_type(oTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				oTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_struct_type(entName.value, reqInfo) ||
			           mod->has_imported_struct_type(entName.value, reqInfo) ||
			           mod->has_struct_type_in_imports(entName.value, reqInfo).first) {
				SHOW("Bring entity is struct")
				auto* cTy = mod->get_struct_type(entName.value, reqInfo);
				if (not cTy->is_accessible(reqInfo)) {
					irCtx->Error("Struct type " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_struct_type(cTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				cTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_choice_type(entName.value, reqInfo) ||
			           mod->has_imported_choice_type(entName.value, reqInfo) ||
			           mod->has_choice_type_in_imports(entName.value, reqInfo).first) {
				auto* chTy = mod->get_choice_type(entName.value, reqInfo);
				if (not chTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Choice type " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_choice_type(chTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				chTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_mix_type(entName.value, reqInfo) ||
			           mod->has_imported_mix_type(entName.value, reqInfo) ||
			           mod->has_mix_type_in_imports(entName.value, reqInfo).first) {
				auto* mTy = mod->get_mix_type(entName.value, reqInfo);
				if (not mTy->is_accessible(reqInfo)) {
					irCtx->Error("Mix type " + irCtx->color(entName.value) + " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_mix_type(mTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				mTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_toggle_type(entName.value, reqInfo) ||
			           mod->has_imported_toggle_type(entName.value, reqInfo) ||
			           mod->has_toggle_type_in_imports(entName.value, reqInfo).first) {
				auto* tgTy = mod->get_toggle_type(entName.value, reqInfo);
				if (not tgTy->is_accessible(reqInfo)) {
					irCtx->Error("Toggle type " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_toggle_type(tgTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				tgTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_type_definition(entName.value, reqInfo) ||
			           mod->has_imported_type_definition(entName.value, reqInfo) ||
			           mod->has_type_definition_in_imports(entName.value, reqInfo).first) {
				auto* dTy = mod->get_type_def(entName.value, reqInfo);
				if (not dTy->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Type definition " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_type_definition(dTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				dTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_region(entName.value, reqInfo) || mod->has_imported_region(entName.value, reqInfo) ||
			           mod->has_region_in_imports(entName.value, reqInfo).first) {
				auto* rTy = mod->get_region(entName.value, reqInfo);
				if (not rTy->is_accessible(reqInfo)) {
					irCtx->Error("Region " + irCtx->color(entName.value) + " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_region(rTy, emitCtx->get_visibility_info(visibSpec), ent->alias);
				rTy->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_function(entName.value, reqInfo) ||
			           mod->has_imported_function(entName.value, reqInfo) ||
			           mod->has_function_in_imports(entName.value, reqInfo).first) {
				auto* otherFn = mod->get_function(entName.value, reqInfo);
				if (not otherFn->is_accessible(reqInfo)) {
					irCtx->Error("Function " + irCtx->color(entName.value) + " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_function(otherFn, emitCtx->get_visibility_info(visibSpec), ent->alias);
				otherFn->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_generic_function(entName.value, reqInfo) ||
			           mod->has_imported_generic_function(entName.value, reqInfo) ||
			           mod->has_generic_function_in_imports(entName.value, reqInfo).first) {
				auto* gnFn = mod->get_generic_function(entName.value, reqInfo);
				if (not gnFn->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Generic function " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_generic_function(gnFn, emitCtx->get_visibility_info(visibSpec), ent->alias);
				gnFn->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_generic_struct_type(entName.value, reqInfo) ||
			           mod->has_imported_generic_struct_type(entName.value, reqInfo) ||
			           mod->has_generic_struct_type_in_imports(entName.value, reqInfo).first) {
				auto* genStruct = mod->get_generic_struct_type(entName.value, reqInfo);
				if (not genStruct->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Generic struct type " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_generic_struct_type(genStruct, emitCtx->get_visibility_info(visibSpec), ent->alias);
				genStruct->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_generic_toggle_type(entName.value, reqInfo) ||
			           mod->has_imported_generic_toggle_type(entName.value, reqInfo) ||
			           mod->has_generic_toggle_type_in_imports(entName.value, reqInfo).first) {
				auto* genTogg = mod->get_generic_toggle_type(entName.value, reqInfo);
				if (not genTogg->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Generic toggle type " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_generic_toggle_type(genTogg, emitCtx->get_visibility_info(visibSpec), ent->alias);
				genTogg->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_global(entName.value, reqInfo) || mod->has_imported_global(entName.value, reqInfo) ||
			           mod->has_global_in_imports(entName.value, reqInfo).first) {
				auto* gEnt = mod->get_global(entName.value, reqInfo);
				if (not gEnt->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Global entity " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_global(gEnt, emitCtx->get_visibility_info(visibSpec), ent->alias);
				gEnt->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_prerun_global(entName.value, reqInfo) ||
			           mod->has_imported_prerun_global(entName.value, reqInfo) ||
			           mod->has_prerun_global_in_imports(entName.value, reqInfo).first) {
				auto* gEnt = mod->get_prerun_global(entName.value, reqInfo);
				if (not gEnt->get_visibility().is_accessible(reqInfo)) {
					irCtx->Error("Prerun global entity " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_prerun_global(gEnt, emitCtx->get_visibility_info(visibSpec), ent->alias);
				gEnt->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (mod->has_prerun_function(entName.value, reqInfo) ||
			           mod->has_imported_prerun_function(entName.value, reqInfo) ||
			           mod->has_prerun_function_in_imports(entName.value, reqInfo).first) {
				auto* preFn = mod->get_prerun_function(entName.value, reqInfo);
				if (not preFn->get_visibility().is_accessible(
				        reqInfo)) { // TODO - Verify that this check is necessary. It most probably isn't. If not remove
					                // this and the above checks as well
					irCtx->Error("Prerun function " + irCtx->color(entName.value) +
					                 " is not accessible in the current scope",
					             entName.range);
				}
				currentMod->import_prerun_function(preFn, emitCtx->get_visibility_info(visibSpec), ent->alias);
				preFn->add_import_mention(currentMod, ent->entity.back().range);
				ent->perform_import();
			} else if (throwErrorsWhenUnfound) {
				irCtx->Error("No module, type, function, region , prerun global, or global named " +
				                 irCtx->color(entName.value) + " found in the parent scope",
				             entName.range);
			}
		}
	};
	for (auto ent : entities) {
		importHandler(ent, currentMod);
	}
	// FIXME - Order of declaration can cause issues
}

Json ImportEntities::to_json() const {
	Vec<JsonValue> entitiesJson;
	for (auto const& ent : entities) {
		entitiesJson.emplace_back(ent->to_json());
	}
	return Json()
	    ._("nodeType", "importEntities")
	    ._("entities", entitiesJson)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

ImportEntities::~ImportEntities() {
	for (auto* ent : entities) {
		std::destroy_at(ent);
	}
}

} // namespace qat::ast
