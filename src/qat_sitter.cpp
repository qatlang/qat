#include "./qat_sitter.hpp"
#include "./show.hpp"
#include "IR/llvm_helper.hpp"
#include "IR/qat_module.hpp"
#include "cli/config.hpp"
#include "cli/error.hpp"
#include "cli/version.hpp"
#include "utils/visibility.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Target/TargetMachine.h"
#include <filesystem>
#include <nuo/json.hpp>

#define OUTPUT_OBJECT_NAME "output"

namespace qat {

QatSitter::QatSitter()
    : Context(new IR::Context()), Lexer(new lexer::Lexer()),
      Parser(new parser::Parser()) {}

void QatSitter::init() {
  auto *config = cli::Config::get();
  auto *ctx    = new IR::Context();
  for (const auto &path : config->getPaths()) {
    handlePath(path, ctx->llctx);
  }
  if (config->shouldExportAST()) {
    for (auto *entity : fileEntities) {
      entity->exportJsonFromAST();
    }
  } else if (config->isCompile()) {
    switch (config->getTarget()) {
    case cli::CompileTarget::normal: {
      for (auto *entity : fileEntities) {
        entity->createModules(ctx);
      }
      for (auto *entity : fileEntities) {
        entity->defineTypes(ctx);
      }
      for (auto *entity : fileEntities) {
        entity->defineNodes(ctx);
      }
      auto *cfg = cli::Config::get();
      if (cfg->hasOutputPath()) {
        fs::remove_all(cfg->getOutputPath() / ".llvm");
      }
      for (auto *entity : fileEntities) {
        ctx->mod = entity;
        SHOW("Calling emitNodes")
        entity->emitNodes(ctx);
      }
      if (checkExecutableExists("clang++")) {
        String compileCommand = "clang++ -fuse-ld=lld ";
        for (const auto &llPath : ctx->llvmOutputPaths) {
          compileCommand += (fs::absolute(llPath).string() + " ");
        }
        if (config->hasOutputPath()) {
          compileCommand +=
              "-o " + (config->getOutputPath() / OUTPUT_OBJECT_NAME).string();
        } else {
          compileCommand += ("-o " OUTPUT_OBJECT_NAME);
        }
        if (!system(compileCommand.c_str())) {
          SHOW("Compiled successfully")
          if (config->isRun() && ctx->hasMain) {
            SHOW("Executing the program")
            if (config->hasOutputPath()) {
              system(
                  (String(".") +
                   ((config->getOutputPath().string().find('/') == 0) ? ""
                                                                      : "/") +
                   (config->getOutputPath() / OUTPUT_OBJECT_NAME).string())
                      .c_str());
            }
          }
        } else {
          cli::Error("Compilation failed", None);
        }
#if IS_RELEASE
        for (const auto &llPath : ctx->llvmOutputPaths) {
          fs::remove(llPath);
        }
        if (cfg->hasOutputPath()) {
          fs::remove_all(cfg->getOutputPath() / ".llvm");
        }
#endif
      } else {
        cli::Error("qat cannot find clang on path. Please make sure that you "
                   "have clang installed and the path to clang is available in "
                   "the environment",
                   None);
      }
      break;
    }
    case cli::CompileTarget::json: {
      // TODO - Implement
    }
    case cli::CompileTarget::cpp: {
      // TODO - Implement
    }
    }
  }
  delete ctx;
}

void QatSitter::handlePath(const fs::path &mainPath, llvm::LLVMContext &llctx) {
  std::function<void(IR::QatModule *, const fs::path &)>
      recursiveModuleCreator = [&](IR::QatModule  *folder,
                                   const fs::path &path) {
        for (auto const &item : path) {
          if (fs::is_directory(item)) {
            if (fs::exists(item / "lib.qat")) {
              Lexer->changeFile(item / "lib.qat");
              Lexer->analyse();
              Parser->setTokens(Lexer->get_tokens());
              fileEntities.push_back(IR::QatModule::CreateFile(
                  folder, fs::absolute(item / "lib.qat"), path, "lib.qat",
                  Lexer->getContent(), Parser->parse(),
                  utils::VisibilityInfo::pub(), llctx));
            } else {
              auto *subfolder = IR::QatModule::CreateSubmodule(
                  folder, item, path, item.filename().string(),
                  IR::ModuleType::folder, utils::VisibilityInfo::pub(), llctx);
              fileEntities.push_back(subfolder);
              recursiveModuleCreator(subfolder, item);
            }
          } else if (fs::is_regular_file(item)) {
            Lexer->changeFile(item.string());
            Lexer->analyse();
            Parser->setTokens(Lexer->get_tokens());
            fileEntities.push_back(IR::QatModule::CreateFile(
                folder, item, path, item.filename().string(),
                Lexer->getContent(), Parser->parse(),
                utils::VisibilityInfo::pub(), llctx));
          }
        }
      };
  if (fs::is_directory(mainPath)) {
    auto libpath = mainPath / "lib.qat";
    if (fs::exists(libpath) && fs::is_regular_file(libpath)) {
      Lexer->changeFile(libpath);
      Lexer->analyse();
      Parser->setTokens(Lexer->get_tokens());
      fileEntities.push_back(IR::QatModule::CreateFile(
          nullptr, libpath, mainPath, "lib.qat", Lexer->getContent(),
          Parser->parse(), utils::VisibilityInfo::pub(), llctx));
    } else {
      auto *subfolder = IR::QatModule::Create(
          mainPath.filename().string(), mainPath, mainPath.parent_path(),
          IR::ModuleType::folder, utils::VisibilityInfo::pub(), llctx);
      fileEntities.push_back(subfolder);
      recursiveModuleCreator(subfolder, mainPath);
    }
  } else if (fs::is_regular_file(mainPath)) {
    Lexer->changeFile(mainPath);
    Lexer->analyse();
    Parser->setTokens(Lexer->get_tokens());
    fileEntities.push_back(IR::QatModule::CreateFile(
        nullptr, fs::absolute(mainPath), mainPath.parent_path(),
        mainPath.filename().string(), Lexer->getContent(), Parser->parse(),
        utils::VisibilityInfo::pub(), llctx));
  }
}

bool QatSitter::checkExecutableExists(const String &name) {
#if PLATFORM_IS_WINDOWS
  LPSTR lpFilePart;
  char  fileName[2000];
  if (!SearchPath(NULL, name, ".exe", 2000, fileName, &lpFilePart)) {
    return false;
  }
  return true;
#else
  if (system(("which " + name).c_str())) {
    return false;
  } else {
    return true;
  }
#endif
}

QatSitter::~QatSitter() {
  for (auto *mod : fileEntities) {
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