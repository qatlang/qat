#include "./bring_paths.hpp"
#include <filesystem>

namespace qat::ast {

void BringPaths::handle_fs_brings(ir::Mod* mod, ir::Ctx* irCtx) const {
  auto emitCtx = EmitCtx::get(irCtx, mod);
  if (visibSpec.has_value()) {
    if (isMember) {
      if (visibSpec->kind != VisibilityKind::pub) {
        irCtx->Error("This is a bring'member sentence and hence cannot have any visibility other than " +
                         irCtx->color("pub"),
                     fileRange);
      }
    }
  }
  for (usize i = 0; i < paths.size(); i++) {
    auto path = fs::path(paths.at(i)->get_value()).is_relative()
                    ? fileRange.file.parent_path() / paths.at(i)->get_value()
                    : fs::path(paths.at(i)->get_value());
    if (fs::exists(path)) {
      path = fs::canonical(path);
      if (fs::is_directory(path)) {
        if (ir::Mod::hasFolderModule(path)) {
          if (names.at(i).has_value()) {
            if (isMember) {
              // FIXME - Maybe change this
              irCtx->Error("This is a bring'member sentence and names are not allowed", names.at(i).value()->fileRange);
            }
            auto const name = Identifier(names.at(i).value()->get_value(), names.at(i).value()->fileRange);
            emitCtx->name_check_in_module(name, "named folder module", None);
            auto* folderModule = ir::Mod::getFolderModule(path);
            folderModule->add_bring_mention(mod, paths.at(i)->fileRange);
            mod->entity_name_check(irCtx, name, ir::EntityType::bringEntity);
            mod->bring_module(folderModule, emitCtx->getVisibInfo(visibSpec), name);
            auto foldEnt          = mod->add_entity(name, ir::EntityType::bringEntity, nullptr, ir::EmitPhase::phase_1);
            foldEnt->currentPhase = ir::EmitPhase::phase_1;
            foldEnt->updateStatus(ir::EntityStatus::complete);
          } else {
            if (isMember) {
              if (ir::Mod::getFolderModule(path)->parent) {
                irCtx->Error("Module at " + irCtx->color(path.string()) + " already has a parent module",
                             paths.at(i)->fileRange);
              }
              auto* folderModule = ir::Mod::getFolderModule(path);
              folderModule->add_fs_bring_mention(mod, paths.at(i)->fileRange);
              mod->addMember(folderModule);
            } else {
              auto* folderModule = ir::Mod::getFolderModule(path);
              folderModule->add_fs_bring_mention(mod, paths.at(i)->fileRange);
              mod->bring_module(folderModule, emitCtx->getVisibInfo(visibSpec));
            }
          }
        } else {
          irCtx->Error("Couldn't find folder module for path: " + irCtx->color(path.string()), fileRange);
        }
      } else if (fs::is_regular_file(path)) {
        if (ir::Mod::hasFileModule(path)) {
          if (names.at(i).has_value()) {
            if (isMember) {
              // FIXME - Maybe change this
              irCtx->Error("This is a " + irCtx->color("bring'member") + " sentence and names are not allowed",
                           names.at(i).value()->fileRange);
            }
            auto const name = Identifier(names.at(i).value()->get_value(), names.at(i).value()->fileRange);
            emitCtx->name_check_in_module(name, "named file module", None);
            auto* fileModule = ir::Mod::getFileModule(path);
            fileModule->add_fs_bring_mention(mod, paths.at(i)->fileRange);
            mod->entity_name_check(irCtx, name, ir::EntityType::bringEntity);
            mod->bring_module(fileModule, emitCtx->getVisibInfo(visibSpec), name);
            auto fileEnt          = mod->add_entity(name, ir::EntityType::bringEntity, nullptr, ir::EmitPhase::phase_1);
            fileEnt->currentPhase = ir::EmitPhase::phase_1;
            fileEnt->updateStatus(ir::EntityStatus::complete);
          } else {
            if (isMember) {
              if (ir::Mod::getFileModule(path)->parent) {
                irCtx->Error("Module at " + irCtx->color(path.string()) + " already has a parent module",
                             paths.at(i)->fileRange);
              }
              auto* fileModule = ir::Mod::getFileModule(path);
              fileModule->add_fs_bring_mention(mod, paths.at(i)->fileRange);
              mod->addMember(fileModule);
            } else {
              auto* fileModule = ir::Mod::getFileModule(path);
              fileModule->add_fs_bring_mention(mod, paths.at(i)->fileRange);
              mod->bring_module(fileModule, emitCtx->getVisibInfo(visibSpec));
            }
          }
        } else {
          irCtx->Error("Couldn't find file module for path: " + irCtx->color(path.string()), fileRange);
        }
      } else {
        irCtx->Error("Cannot bring this file type", paths.at(i)->fileRange);
      }
    } else {
      irCtx->Error("The path provided does not exist: " + path.string() + " and cannot be brought in.",
                   paths.at(i)->fileRange);
    }
  }
}

Json BringPaths::to_json() const {
  Vec<JsonValue> pths;
  for (auto* path : paths) {
    pths.push_back(path->to_json());
  }
  return Json()
      ._("nodeType", "bringPaths")
      ._("paths", std::move(pths))
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast