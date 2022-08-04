#include "./qat_sitter.hpp"
#include "./show.hpp"
#include "IR/qat_module.hpp"
#include "cli/config.hpp"
#include "utils/visibility.hpp"
#include <filesystem>
#include <nuo/json.hpp>

namespace qat {

QatSitter::QatSitter()
    : Context(new IR::Context()), Parser(new parser::Parser()),
      Lexer(new lexer::Lexer()) {}

void QatSitter::init() {
  auto config = cli::Config::get();
  for (auto path : config->getPaths()) {
    handlePath(path);
  }
  if (config->shouldExportAST()) {
    for (auto entity : fileEntities) {
      entity->exportJsonFromAST();
    }
  } else if (config->isCompile()) {
    switch (config->getTarget()) {
    case cli::CompileTarget::normal: {
    }
    case cli::CompileTarget::json: {
    }
    case cli::CompileTarget::cpp: {
    }
    }
  }
}

void QatSitter::handlePath(fs::path path) {
  std::function<void(IR::QatModule *, fs::path)> recursiveModuleCreator =
      [&](IR::QatModule *folder, fs::path path) {
        for (auto const &item : path) {
          if (fs::is_directory(item)) {
            if (fs::exists(item / "lib.qat")) {
              Lexer->changeFile(item / "lib.qat");
              Lexer->analyse();
              Parser->setTokens(Lexer->get_tokens());
              fileEntities.push_back(IR::QatModule::CreateFile(
                  folder, fs::absolute(item / "lib.qat"), path, "lib.qat",
                  Parser->parse(), utils::VisibilityInfo::pub()));
            } else {
              auto subfolder = IR::QatModule::CreateSubmodule(
                  folder, item, path, item.filename().string(),
                  IR::ModuleType::folder, utils::VisibilityInfo::pub());
              fileEntities.push_back(subfolder);
              recursiveModuleCreator(subfolder, item);
            }
          } else if (fs::is_regular_file(item)) {
            Lexer->changeFile(item.string());
            Lexer->analyse();
            Parser->setTokens(Lexer->get_tokens());
            fileEntities.push_back(IR::QatModule::CreateFile(
                folder, item, path, item.filename().string(), Parser->parse(),
                utils::VisibilityInfo::pub()));
          }
        }
      };
  if (fs::is_directory(path)) {
    auto libpath = path / "lib.qat";
    if (fs::exists(libpath) && fs::is_regular_file(libpath)) {
      Lexer->changeFile(libpath);
      Lexer->analyse();
      Parser->setTokens(Lexer->get_tokens());
      fileEntities.push_back(IR::QatModule::CreateFile(
          nullptr, libpath, path, "lib.qat", Parser->parse(),
          utils::VisibilityInfo::pub()));
    } else {
      auto subfolder = IR::QatModule::Create(
          path.filename().string(), path, path.parent_path(),
          IR::ModuleType::folder, utils::VisibilityInfo::pub());
      fileEntities.push_back(subfolder);
      recursiveModuleCreator(subfolder, path);
    }
  } else if (fs::is_regular_file(path)) {
    Lexer->changeFile(path);
    Lexer->analyse();
    Parser->setTokens(Lexer->get_tokens());
    fileEntities.push_back(IR::QatModule::CreateFile(
        nullptr, fs::absolute(path), path.parent_path(),
        path.filename().string(), Parser->parse(),
        utils::VisibilityInfo::pub()));
  }
}

QatSitter::~QatSitter() {
  for (auto mod : fileEntities) {
    delete mod;
  }
  fileEntities.clear();
  delete Context;
  Context = nullptr;
  delete Parser;
  Parser = nullptr;
  delete Lexer;
  Lexer = nullptr;
}

} // namespace qat