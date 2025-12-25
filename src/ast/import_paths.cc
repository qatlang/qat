#include "./import_paths.hpp"
#include <filesystem>

namespace qat::ast {

void ImportPaths::handle_filesystem_imports(ir::Mod* mod, ir::Ctx* irCtx) const {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	if (visibSpec.has_value()) {
		if (isMember) {
			if (visibSpec->kind != VisibilityKind::pub) {
				irCtx->Error("This is a " + irCtx->color("use:member") +
				                 " statement and hence cannot have any visibility other than " + irCtx->color("pub"),
				             fileRange);
			}
		}
	}
	for (usize i = 0; i < paths.size(); i++) {
		auto path = fs::path(paths.at(i)->get_value()).is_relative()
		                ? fs::path(*fileRange->file).parent_path() / paths.at(i)->get_value()
		                : fs::path(paths.at(i)->get_value());
		if (fs::exists(path)) {
			path = fs::canonical(path);
			if (fs::is_directory(path)) {
				if (ir::Mod::has_folder_module(path)) {
					if (names.at(i).has_value()) {
						if (isMember) {
							// FIXME - Maybe change this
							irCtx->Error("This is a " + irCtx->color("use:member") +
							                 " statement and alias is not allowed here",
							             names.at(i).value()->fileRange);
						}
						auto const name = Identifier(names.at(i).value()->get_value(), names.at(i).value()->fileRange);
						emitCtx->name_check_in_module(name, "named folder module", None);
						auto* folderModule = ir::Mod::get_folder_module(path);
						folderModule->add_import_mention(mod, paths.at(i)->fileRange);
						mod->entity_name_check(irCtx, name, ir::EntityType::importEntity);
						mod->import_module(folderModule, emitCtx->get_visibility_info(visibSpec), name);
						auto foldEnt =
						    mod->add_entity(name, ir::EntityType::importEntity, nullptr, ir::EmitPhase::phase_1);
						foldEnt->currentPhase = ir::EmitPhase::phase_1;
						foldEnt->updateStatus(ir::EntityStatus::complete);
					} else {
						if (isMember) {
							if (ir::Mod::get_folder_module(path)->parent) {
								irCtx->Error("Module at " + irCtx->color(path.string()) +
								                 " already has a parent module",
								             paths.at(i)->fileRange);
							}
							auto* folderModule = ir::Mod::get_folder_module(path);
							folderModule->add_filesystem_import_mention(mod, paths.at(i)->fileRange);
							mod->addMember(folderModule);
						} else {
							auto* folderModule = ir::Mod::get_folder_module(path);
							folderModule->add_filesystem_import_mention(mod, paths.at(i)->fileRange);
							mod->import_module(folderModule, emitCtx->get_visibility_info(visibSpec));
						}
					}
				} else {
					irCtx->Error("Couldn't find folder module for path: " + irCtx->color(path.string()), fileRange);
				}
			} else if (fs::is_regular_file(path)) {
				if (ir::Mod::has_file_module(path)) {
					if (names.at(i).has_value()) {
						if (isMember) {
							// FIXME - Maybe change this
							irCtx->Error("This is a " + irCtx->color("use:member") +
							                 " statement and alias is not allowed here",
							             names.at(i).value()->fileRange);
						}
						auto const name = Identifier(names.at(i).value()->get_value(), names.at(i).value()->fileRange);
						emitCtx->name_check_in_module(name, "named file module", None);
						auto* fileModule = ir::Mod::get_file_module(path);
						fileModule->add_filesystem_import_mention(mod, paths.at(i)->fileRange);
						mod->entity_name_check(irCtx, name, ir::EntityType::importEntity);
						mod->import_module(fileModule, emitCtx->get_visibility_info(visibSpec), name);
						auto fileEnt =
						    mod->add_entity(name, ir::EntityType::importEntity, nullptr, ir::EmitPhase::phase_1);
						fileEnt->currentPhase = ir::EmitPhase::phase_1;
						fileEnt->updateStatus(ir::EntityStatus::complete);
					} else {
						if (isMember) {
							if (ir::Mod::get_file_module(path)->parent) {
								irCtx->Error("Module at " + irCtx->color(path.string()) +
								                 " already has a parent module",
								             paths.at(i)->fileRange);
							}
							auto* fileModule = ir::Mod::get_file_module(path);
							fileModule->add_filesystem_import_mention(mod, paths.at(i)->fileRange);
							mod->addMember(fileModule);
						} else {
							auto* fileModule = ir::Mod::get_file_module(path);
							fileModule->add_filesystem_import_mention(mod, paths.at(i)->fileRange);
							mod->import_module(fileModule, emitCtx->get_visibility_info(visibSpec));
						}
					}
				} else {
					irCtx->Error("Couldn't find file module for path: " + irCtx->color(path.string()), fileRange);
				}
			} else {
				irCtx->Error("Cannot import this file type", paths.at(i)->fileRange);
			}
		} else {
			irCtx->Error("The path provided does not exist: " + path.string() + " and cannot be imported",
			             paths.at(i)->fileRange);
		}
	}
}

Json ImportPaths::to_json() const {
	Vec<JsonValue> pathsJSON;
	for (auto* path : paths) {
		pathsJSON.push_back(path->to_json());
	}
	return Json()
	    ._("nodeType", "importPaths")
	    ._("paths", std::move(pathsJSON))
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
