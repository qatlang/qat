#include "./qat_sitter.hpp"
#include "./show.hpp"
#include "IR/qat_module.hpp"
#include "cli/config.hpp"
#include "cli/error.hpp"
#include "utils/visibility.hpp"
#include "llvm/IR/LLVMContext.h"
#include <algorithm>
#include <chrono>
#include <filesystem>

#define OUTPUT_OBJECT_NAME "output"

namespace qat {

QatSitter::QatSitter() : Context(new IR::Context()), Lexer(new lexer::Lexer()), Parser(new parser::Parser()) {}

void QatSitter::init() {
  auto* config = cli::Config::get();
  auto* ctx    = new IR::Context();
  for (const auto& path : config->getPaths()) {
    handlePath(path, ctx->llctx);
  }
  if (config->shouldExportAST()) {
    for (auto* entity : fileEntities) {
      entity->exportJsonFromAST();
    }
  } else if (config->isCompile() || config->isAnalyse()) {
    ctx->qatStartTime = std::chrono::steady_clock::now();
    for (auto* entity : fileEntities) {
      entity->createModules(ctx);
    }
    for (auto* entity : fileEntities) {
      entity->handleBrings(ctx);
    }
    for (auto* entity : fileEntities) {
      entity->defineTypes(ctx);
    }
    for (auto* entity : fileEntities) {
      entity->defineNodes(ctx);
    }
    auto* cfg = cli::Config::get();
    if (cfg->hasOutputPath()) {
      fs::remove_all(cfg->getOutputPath() / "llvm");
    }
    for (auto* entity : fileEntities) {
      ctx->mod = entity;
      SHOW("Calling emitNodes")
      entity->emitNodes(ctx);
    }
    SHOW("Emitted nodes")
    ctx->qatEndTime         = std::chrono::steady_clock::now();
    ctx->clangLinkStartTime = std::chrono::steady_clock::now();
    if (cfg->isCompile()) {
      if (checkExecutableExists("clang")) {
        for (auto* entity : fileEntities) {
          entity->compileToObject(ctx);
        }
        SHOW("Modules compiled to object format")
        for (auto* entity : fileEntities) {
          entity->bundleLibs(ctx);
        }
        ctx->clangLinkEndTime = std::chrono::steady_clock::now();
        ctx->writeJsonResult(true);
#if IS_RELEASE
        for (const auto& llPath : ctx->llvmOutputPaths) {
          fs::remove(llPath);
        }
        if (cfg->hasOutputPath()) {
          fs::remove_all(cfg->getOutputPath() / "llvm");
        }
#endif
      } else {
        ctx->writeJsonResult(false);
        cli::Error("qat cannot find clang on path. Please make sure that you "
                   "have clang installed and the path to clang is available in "
                   "the environment",
                   None);
      }
    } else {
      ctx->writeJsonResult(true);
    }
  }
  delete ctx;
}

void QatSitter::queuePath(fs::path path) { queuedPaths.push_back(std::move(path)); }

void QatSitter::handlePath(const fs::path& mainPath, llvm::LLVMContext& llctx) {
  Vec<fs::path> broughtPaths;
  SHOW("Handling path: " << mainPath.string())
  std::function<void(IR::QatModule*, const fs::path&)> recursiveModuleCreator = [&](IR::QatModule*  folder,
                                                                                    const fs::path& path) {
    for (auto const& item : path) {
      if (fs::is_directory(item) && !IR::QatModule::hasFolderModule(item)) {
        auto libPath = item / "lib.qat";
        if (fs::exists(libPath)) {
          Lexer->changeFile(fs::absolute(libPath));
          Lexer->analyse();
          Parser->setTokens(Lexer->get_tokens());
          auto parseRes(Parser->parse());
          for (const auto& bPath : Parser->getBroughtPaths()) {
            broughtPaths.push_back(bPath);
          }
          Parser->clearBroughtPaths();
          fileEntities.push_back(IR::QatModule::CreateFile(folder, fs::absolute(libPath), path, "lib.qat",
                                                           Lexer->getContent(), std::move(parseRes),
                                                           utils::VisibilityInfo::pub(), llctx));
        } else {
          auto* subfolder = IR::QatModule::CreateSubmodule(folder, item, path, fs::absolute(item.filename()).string(),
                                                           IR::ModuleType::folder, utils::VisibilityInfo::pub(), llctx);
          fileEntities.push_back(subfolder);
          recursiveModuleCreator(subfolder, item);
        }
      } else if (fs::is_regular_file(item) && !IR::QatModule::hasFileModule(item)) {
        Lexer->changeFile(item.string());
        Lexer->analyse();
        Parser->setTokens(Lexer->get_tokens());
        auto parseRes(Parser->parse());
        for (const auto& bPath : Parser->getBroughtPaths()) {
          broughtPaths.push_back(bPath);
        }
        Parser->clearBroughtPaths();
        fileEntities.push_back(IR::QatModule::CreateFile(folder, fs::absolute(item), path, item.filename().string(),
                                                         Lexer->getContent(), std::move(parseRes),
                                                         utils::VisibilityInfo::pub(), llctx));
      }
    }
  };
  SHOW("Created recursive module creator")
  if (fs::is_directory(mainPath) && !IR::QatModule::hasFolderModule(mainPath)) {
    SHOW("Is directory")
    auto libpath = mainPath / "lib.qat";
    if (fs::exists(libpath) && fs::is_regular_file(libpath)) {
      Lexer->changeFile(libpath);
      Lexer->analyse();
      Parser->setTokens(Lexer->get_tokens());
      auto parseRes(Parser->parse());
      for (const auto& bPath : Parser->getBroughtPaths()) {
        broughtPaths.push_back(bPath);
      }
      Parser->clearBroughtPaths();
      fileEntities.push_back(IR::QatModule::CreateFile(nullptr, libpath, mainPath, "lib.qat", Lexer->getContent(),
                                                       std::move(parseRes), utils::VisibilityInfo::pub(), llctx));
    } else {
      auto* subfolder = IR::QatModule::Create(mainPath.filename().string(), mainPath, mainPath.parent_path(),
                                              IR::ModuleType::folder, utils::VisibilityInfo::pub(), llctx);
      fileEntities.push_back(subfolder);
      recursiveModuleCreator(subfolder, mainPath);
    }
  } else if (fs::is_regular_file(mainPath) && !IR::QatModule::hasFileModule(mainPath)) {
    SHOW("Is regular file")
    Lexer->changeFile(mainPath);
    Lexer->analyse();
    Parser->setTokens(Lexer->get_tokens());
    auto parseRes(Parser->parse());
    for (const auto& bPath : Parser->getBroughtPaths()) {
      broughtPaths.push_back(bPath);
    }
    Parser->clearBroughtPaths();
    fileEntities.push_back(IR::QatModule::CreateFile(nullptr, fs::absolute(mainPath), mainPath.parent_path(),
                                                     mainPath.filename().string(), Lexer->getContent(),
                                                     std::move(parseRes), utils::VisibilityInfo::pub(), llctx));
  }
  for (const auto& bPath : broughtPaths) {
    handlePath(bPath, llctx);
  }
  broughtPaths.clear();
}

bool QatSitter::checkExecutableExists(const String& name) {
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
  for (auto* mod : fileEntities) {
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