#include "./bring_paths.hpp"
#include <filesystem>

namespace qat::ast {

BringPaths::BringPaths(bool _isMember, Vec<StringLiteral*> _paths, Vec<Maybe<StringLiteral*>> _names,
                       Maybe<utils::VisibilityKind> _visibility, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), isMember(_isMember), paths(std::move(_paths)), visibility(_visibility),
      names(std::move(_names)) {}

void BringPaths::handleBrings(IR::Context* ctx) const {
  auto* mod = ctx->getMod();
  if (visibility.has_value()) {
    if (isMember) {
      if (visibility.value() != utils::VisibilityKind::pub) {
        ctx->Error("This is a bring'member sentence and hence cannot have any visibility other than " +
                       ctx->highlightError("pub"),
                   fileRange);
      }
    }
  }
  for (usize i = 0; i < paths.size(); i++) {
    auto path = fileRange.file.parent_path() / paths.at(i)->get_value();
    if (fs::exists(path)) {
      if (fs::is_directory(path)) {
        if (IR::QatModule::hasFolderModule(path)) {
          if (names.at(i).has_value()) {
            if (isMember) {
              // FIXME - Maybe change this
              ctx->Error("This is a bring'member sentence and names are not allowed", names.at(i).value()->fileRange);
            }
            auto name = names.at(i).value()->get_value();
            if (mod->hasLib(name)) {
              ctx->Error("A lib named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBox(name)) {
              ctx->Error("A box named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBroughtLib(name)) {
              ctx->Error("A brought lib named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBroughtBox(name)) {
              ctx->Error("A brought box named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasAccessibleBoxInImports(name, ctx->getReqInfo()).first) {
              ctx->Error("A box named " + ctx->highlightError(name) + " is already accessible in this scope",
                         names.at(i).value()->fileRange);
            } else if (mod->hasAccessibleLibInImports(name, ctx->getReqInfo()).first) {
              ctx->Error("A lib named " + ctx->highlightError(name) + " is already accessible in this scope",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBroughtModule(name)) {
              ctx->Error("A brought module named " + ctx->highlightError(name) + " is in this scope",
                         names.at(i).value()->fileRange);
            } else if (mod->hasAccessibleBroughtModuleInImports(name, ctx->getReqInfo()).first) {
              ctx->Error("A brought module named " + ctx->highlightError(name) + " is already accessible in this scope",
                         names.at(i).value()->fileRange);
            }
            auto* folderModule = IR::QatModule::getFolderModule(path);
            folderModule->addMention(mod, paths.at(i)->fileRange);
            mod->bringNamedModule(name, folderModule, ctx->getVisibInfo(visibility));
          } else {
            if (isMember) {
              auto* folderModule = IR::QatModule::getFolderModule(path);
              folderModule->addMention(mod, paths.at(i)->fileRange);
              mod->addMember(folderModule);
            } else {
              auto* folderModule = IR::QatModule::getFolderModule(path);
              folderModule->addMention(mod, paths.at(i)->fileRange);
              mod->bringModule(folderModule, ctx->getVisibInfo(visibility));
            }
          }
        } else {
          ctx->Error("Couldn't find folder module for path: " + ctx->highlightError(path.string()), fileRange);
        }
      } else if (fs::is_regular_file(path)) {
        if (IR::QatModule::hasFileModule(path)) {
          if (names.at(i).has_value()) {
            if (isMember) {
              // FIXME - Maybe change this
              ctx->Error("This is a bring'member sentence and names are not allowed", names.at(i).value()->fileRange);
            }
            auto name = names.at(i).value()->get_value();
            if (mod->hasLib(name)) {
              ctx->Error("A lib named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBox(name)) {
              ctx->Error("A box named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBroughtLib(name)) {
              ctx->Error("A brought lib named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBroughtBox(name)) {
              ctx->Error("A brought box named " + ctx->highlightError(name) + " exists already",
                         names.at(i).value()->fileRange);
            } else if (mod->hasAccessibleBoxInImports(name, ctx->getReqInfo()).first) {
              ctx->Error("A box named " + ctx->highlightError(name) + " is already accessible in this scope",
                         names.at(i).value()->fileRange);
            } else if (mod->hasAccessibleLibInImports(name, ctx->getReqInfo()).first) {
              ctx->Error("A lib named " + ctx->highlightError(name) + " is already accessible in this scope",
                         names.at(i).value()->fileRange);
            } else if (mod->hasBroughtModule(name)) {
              ctx->Error("A brought module named " + ctx->highlightError(name) + " is in this scope",
                         names.at(i).value()->fileRange);
            } else if (mod->hasAccessibleBroughtModuleInImports(name, ctx->getReqInfo()).first) {
              ctx->Error("A brought module named " + ctx->highlightError(name) + " is already accessible in this scope",
                         names.at(i).value()->fileRange);
            }
            auto* fileModule = IR::QatModule::getFileModule(path);
            fileModule->addMention(mod, paths.at(i)->fileRange);
            mod->bringNamedModule(name, fileModule, ctx->getVisibInfo(visibility));
          } else {
            if (isMember) {
              auto* fileModule = IR::QatModule::getFileModule(path);
              fileModule->addMention(mod, paths.at(i)->fileRange);
              mod->addMember(fileModule);
            } else {
              auto* fileModule = IR::QatModule::getFileModule(path);
              fileModule->addMention(mod, paths.at(i)->fileRange);
              mod->bringModule(fileModule, ctx->getVisibInfo(visibility));
            }
          }
        } else {
          ctx->Error("Couldn't find file module for path: " + ctx->highlightError(path.string()), fileRange);
        }
      } else {
        ctx->Error("Cannot bring this file type", paths.at(i)->fileRange);
      }
    } else {
      ctx->Error("The path provided does not exist: " + path.string() + " and cannot be brought in.",
                 paths.at(i)->fileRange);
    }
  }
}

Json BringPaths::toJson() const {
  Vec<JsonValue> pths;
  for (auto* path : paths) {
    pths.push_back(path->toJson());
  }
  return Json()
      ._("nodeType", "bringPaths")
      ._("paths", std::move(pths))
      ._("hasVisibility", visibility.has_value())
      ._("visibility", visibility.has_value() ? utils::kindToJsonValue(visibility.value()) : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast