#include "./qat_sitter.hpp"
#include "./memory_tracker.hpp"
#include "./show.hpp"
#include "IR/qat_module.hpp"
#include "IR/value.hpp"
#include "ast/types/qat_type.hpp"
#include "cli/config.hpp"
#include "cli/error.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token_type.hpp"
#include "parser/parser.hpp"
#include "utils/identifier.hpp"
#include "utils/logger.hpp"
#include "utils/qat_region.hpp"
#include "utils/visibility.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/HashBuilder.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <ios>
#include <system_error>

#if PlatformIsWindows
#include "windows.h"
#endif

#define OUTPUT_OBJECT_NAME "output"

namespace qat {

QatSitter* QatSitter::instance = nullptr;

QatSitter::QatSitter() : ctx(IR::Context::New()), Lexer(lexer::Lexer::get(ctx)), Parser(parser::Parser::get(ctx)) {
  ctx->sitter = this;
}

QatSitter* QatSitter::get() {
  if (instance) {
    return instance;
  }
  instance = std::construct_at(OwnTracked(QatSitter));
  return instance;
}

void QatSitter::initialise() {
  auto* config = cli::Config::get();
  for (const auto& path : config->getPaths()) {
    handlePath(path, ctx);
  }
  if (config->hasStdLibPath() && ctx->stdLibRequired) {
    handlePath(config->getStdLibPath(), ctx);
    if (IR::QatModule::hasFileModule(config->getStdLibPath())) {
      IR::QatModule::stdLibModule = IR::QatModule::getFileModule(config->getStdLibPath());
    }
  }
  if (config->isCompile() || config->isAnalyse()) {
    ctx->qatStartTime = std::chrono::high_resolution_clock::now();
    for (auto* entity : fileEntities) {
      entity->createModules(ctx);
    }
    for (auto* entity : fileEntities) {
      entity->handleFilesystemBrings(ctx);
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
    for (auto* entity : fileEntities) {
      entity->handleBrings(ctx);
    }
    auto* cfg = cli::Config::get();
    if (cfg->hasOutputPath()) {
      fs::remove_all(cfg->getOutputPath() / "llvm");
    }
    for (auto* entity : fileEntities) {
      auto* oldMod = ctx->setActiveModule(entity);
      entity->emitNodes(ctx);
      (void)ctx->setActiveModule(oldMod);
    }
    SHOW("Emitted nodes")
    ctx->qatEndTime = std::chrono::high_resolution_clock::now();
    //
    //
    if (cfg->exportCodeMetadata()) {
      SHOW("About to export code metadata")
      // NOLINTNEXTLINE(readability-isolate-declaration)
      Vec<JsonValue> modulesJson, functionsJson, genericFunctionsJson, genericCoreTypesJson, coreTypesJson,
          mixTypesJson, regionJson, choiceJson, defsJson;
      for (auto* entity : fileEntities) {
        entity->outputAllOverview(modulesJson, functionsJson, genericFunctionsJson, genericCoreTypesJson, coreTypesJson,
                                  mixTypesJson, regionJson, choiceJson, defsJson);
      }
      auto            codeStructFilePath = cfg->getOutputPath() / "QatCodeInfo.json";
      bool            codeInfoExists     = fs::exists(codeStructFilePath);
      std::error_code errorCode;
      if (!codeInfoExists) {
        fs::create_directories(codeStructFilePath.parent_path(), errorCode);
      } else {
        fs::remove(codeStructFilePath);
      }
      if (!errorCode || codeInfoExists) {
        std::ofstream mStream;
        mStream.open(codeStructFilePath, std::ios_base::out);
        if (mStream.is_open()) {
          mStream << Json()
                         ._("modules", modulesJson)
                         ._("functions", functionsJson)
                         ._("genericFunctions", genericFunctionsJson)
                         ._("coreTypes", coreTypesJson)
                         ._("genericCoreTypes", genericCoreTypesJson)
                         ._("mixTypes", mixTypesJson)
                         ._("regions", regionJson)
                         ._("choiceTypes", choiceJson)
                         ._("typeDefinitions", defsJson);
          mStream.close();
        } else {
          ctx->Error("Could not open the code info file for output", codeStructFilePath);
        }
      } else {
        ctx->Error("Could not create parent directory of the code info file", {codeStructFilePath});
      }
    }
    //
    //
    SHOW(cfg->shouldExportAST())
    if (cfg->shouldExportAST()) {
      for (auto* entity : fileEntities) {
        entity->exportJsonFromAST(ctx);
      }
    }
    SHOW("Getting link start time")
    auto clangStartTime = std::chrono::high_resolution_clock::now();
    auto clearLLVM      = [&] {
      if (cfg->clearLLVM()) {
        for (const auto& llPath : ctx->llvmOutputPaths) {
          fs::remove(llPath);
        }
        if (cfg->hasOutputPath() && fs::exists(cfg->getOutputPath() / "llvm")) {
          fs::remove_all(cfg->getOutputPath() / "llvm");
        }
      }
    };
    if (cfg->isCompile()) {
      SHOW("Checking whether clang exists or not")
      if (checkExecutableExists("clang") || checkExecutableExists("clang++")) {
        for (auto* entity : fileEntities) {
          entity->compileToObject(ctx);
        }
        SHOW("Modules compiled to object format")
        for (auto* entity : fileEntities) {
          entity->bundleLibs(ctx);
        }
        ctx->clangLinkEndTime = std::chrono::high_resolution_clock::now();
        ctx->writeJsonResult(true);
        clearLLVM();
        if (cfg->isRun() && !ctx->executablePaths.empty()) {
          bool hasMultiExecutables = ctx->executablePaths.size() > 1;
          for (const auto& exePath : ctx->executablePaths) {
            SHOW("Running built executable at: " << exePath.string())
            if (hasMultiExecutables) {
              std::cout << "\nExecuting built executable at: " << exePath.string() << "\n";
            }
            if (std::system(exePath.string().c_str())) {
              ctx->Error("\nThe built executable at " + ctx->highlightError(exePath.string()) +
                             " exited with an error!",
                         None);
            }
          }
          std::cout << "\n\n";
          SHOW("Ran all compiled executables")
        }
      } else {
        ctx->Error("qat cannot find clang on path. Please make sure that you have clang installed and the "
                   "path to clang is available in the environment",
                   None);
      }
    } else {
      clearLLVM();
      ctx->writeJsonResult(true);
    }
  }
}

void QatSitter::removeEntityWithPath(const fs::path& path) {
  for (auto item = fileEntities.begin(); item != fileEntities.end(); item++) {
    if (((*item)->getModuleType() == IR::ModuleType::file || (*item)->getModuleType() == IR::ModuleType::folder) &&
        fs::equivalent(fs::path((*item)->getFilePath()), path)) {
      fileEntities.erase(item);
      return;
    }
  }
}

Maybe<Pair<String, fs::path>> QatSitter::detectLibFile(const fs::path& path) {
  if (fs::is_directory(path)) {
    SHOW("Path is a directory: " << fs::absolute(path).string())
    for (const auto& item : fs::directory_iterator(path)) {
      if (fs::is_regular_file(item)) {
        auto name = item.path().filename().string();
        if (name.ends_with(".lib.qat")) {
          SHOW("lib file detected: " << name.substr(0, name.length() - 8))
          return Pair<String, fs::path>(name.substr(0, name.length() - 8), item); // NOLINT(readability-magic-numbers)
        }
      }
    }
  } else if (fs::is_regular_file(path)) {
    SHOW("Path is a file: " << path.string())
    auto name = path.filename().string();
    if (name.ends_with(".lib.qat")) {
      SHOW("lib file detected: " << name.substr(0, name.length() - 8))
      return Pair<String, fs::path>(name.substr(0, name.length() - 8), path); // NOLINT(readability-magic-numbers)
    }
  }
  return None;
}

bool QatSitter::isNameValid(const String& name) {
  return (lexer::Lexer::wordToToken(name, nullptr).type == lexer::TokenType::identifier);
}

void QatSitter::handlePath(const fs::path& mainPath, IR::Context* ctx) {
  Vec<fs::path> broughtPaths;
  Vec<fs::path> memberPaths;
  auto*         cfg = cli::Config::get();
  SHOW("Handling path: " << mainPath.string())
  std::function<void(IR::QatModule*, const fs::path&)> recursiveModuleCreator = [&](IR::QatModule*  parentMod,
                                                                                    const fs::path& path) {
    for (auto const& item : fs::directory_iterator(path)) {
      if (fs::is_directory(item) && !fs::equivalent(item, cfg->getOutputPath()) &&
          !IR::QatModule::hasFolderModule(item)) {
        auto libCheckRes = detectLibFile(item);
        if (libCheckRes.has_value()) {
          if (!isNameValid(libCheckRes->first)) {
            ctx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
                           ctx->highlightError(libCheckRes->first) + " which is illegal",
                       None);
          }
          Lexer->changeFile(fs::absolute(libCheckRes->second));
          Lexer->analyse();
          Parser->setTokens(Lexer->getTokens());
          auto parseRes(Parser->parse());
          for (const auto& bPath : Parser->getBroughtPaths()) {
            broughtPaths.push_back(bPath);
          }
          for (const auto& mPath : Parser->getMemberPaths()) {
            memberPaths.push_back(mPath);
          }
          Parser->clearBroughtPaths();
          Parser->clearMemberPaths();
          fileEntities.push_back(IR::QatModule::CreateRootLib(
              parentMod, fs::absolute(libCheckRes->second), path, Identifier(libCheckRes->first, libCheckRes->second),
              Lexer->getContent(), std::move(parseRes), VisibilityInfo::pub(), ctx));
        } else {
          auto dirQatChecker = [](const fs::directory_entry& entry) {
            bool foundQatFile = false;
            for (auto const& dirItem : fs::directory_iterator(entry)) {
              if (dirItem.is_regular_file() && dirItem.path().extension() == ".qat") {
                foundQatFile = true;
                break;
              }
            }
            return foundQatFile;
          };
          if (dirQatChecker(item)) {
            auto* subfolder = IR::QatModule::CreateSubmodule(
                parentMod, item.path(), path, Identifier(fs::absolute(item.path().filename()).string(), item.path()),
                IR::ModuleType::folder, VisibilityInfo::pub(), ctx);
            fileEntities.push_back(subfolder);
            recursiveModuleCreator(subfolder, item);
          } else {
            recursiveModuleCreator(parentMod, item);
          }
        }
      } else if (fs::is_regular_file(item) && !IR::QatModule::hasFileModule(item) &&
                 (item.path().extension() == ".qat")) {
        auto libCheckRes = detectLibFile(item);
        if (libCheckRes.has_value() && !isNameValid(libCheckRes.value().first)) {
          ctx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
                         ctx->highlightError(libCheckRes->first) + " which is illegal",
                     None);
        }
        Lexer->changeFile(item.path().string());
        Lexer->analyse();
        Parser->setTokens(Lexer->getTokens());
        auto parseRes(Parser->parse());
        for (const auto& bPath : Parser->getBroughtPaths()) {
          broughtPaths.push_back(bPath);
        }
        for (const auto& mPath : Parser->getMemberPaths()) {
          memberPaths.push_back(mPath);
        }
        Parser->clearBroughtPaths();
        Parser->clearMemberPaths();
        if (libCheckRes.has_value()) {
          fileEntities.push_back(IR::QatModule::CreateRootLib(
              parentMod, fs::absolute(item), path, Identifier(libCheckRes->first, libCheckRes->second),
              Lexer->getContent(), std::move(parseRes), VisibilityInfo::pub(), ctx));
        } else {
          fileEntities.push_back(IR::QatModule::CreateFileMod(
              parentMod, fs::absolute(item), path, Identifier(item.path().filename().string(), item.path()),
              Lexer->getContent(), std::move(parseRes), VisibilityInfo::pub(), ctx));
        }
      }
    }
  };
  // FIXME - Check if modules are already part of another module
  SHOW("Created recursive module creator")
  if (fs::is_directory(mainPath) && !fs::equivalent(mainPath, cfg->getOutputPath()) &&
      !IR::QatModule::hasFolderModule(mainPath)) {
    SHOW("Is directory")
    auto libCheckRes = detectLibFile(mainPath);
    if (libCheckRes.has_value()) {
      if (!isNameValid(libCheckRes.value().first)) {
        ctx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
                       ctx->highlightError(libCheckRes->first) + " which is illegal",
                   None);
      }
      Lexer->changeFile(libCheckRes->second);
      Lexer->analyse();
      Parser->setTokens(Lexer->getTokens());
      auto parseRes(Parser->parse());
      for (const auto& bPath : Parser->getBroughtPaths()) {
        broughtPaths.push_back(bPath);
      }
      for (const auto& mPath : Parser->getMemberPaths()) {
        memberPaths.push_back(mPath);
      }
      Parser->clearBroughtPaths();
      Parser->clearMemberPaths();
      fileEntities.push_back(IR::QatModule::CreateFileMod(
          nullptr, libCheckRes->second, mainPath, Identifier(libCheckRes->first, libCheckRes->second),
          Lexer->getContent(), std::move(parseRes), VisibilityInfo::pub(), ctx));
    } else {
      auto* subfolder =
          IR::QatModule::Create(Identifier(mainPath.filename().string(), mainPath), mainPath, mainPath.parent_path(),
                                IR::ModuleType::folder, VisibilityInfo::pub(), ctx);
      fileEntities.push_back(subfolder);
      recursiveModuleCreator(subfolder, mainPath);
    }
  } else if (fs::is_regular_file(mainPath) && !IR::QatModule::hasFileModule(mainPath)) {
    auto libCheckRes = detectLibFile(mainPath);
    if (libCheckRes.has_value() && !isNameValid(libCheckRes.value().first)) {
      ctx->Error("The name of the library file " + libCheckRes->second.string() + " is " +
                     ctx->highlightError(libCheckRes->first) + " which is illegal",
                 None);
    }
    SHOW("Is regular file")
    Lexer->changeFile(mainPath);
    Lexer->analyse();
    Parser->setTokens(Lexer->getTokens());
    auto parseRes(Parser->parse());
    for (const auto& bPath : Parser->getBroughtPaths()) {
      broughtPaths.push_back(bPath);
    }
    for (const auto& mPath : Parser->getMemberPaths()) {
      memberPaths.push_back(mPath);
    }
    Parser->clearBroughtPaths();
    Parser->clearMemberPaths();
    if (libCheckRes.has_value()) {
      fileEntities.push_back(IR::QatModule::CreateRootLib(
          nullptr, fs::absolute(mainPath), mainPath.parent_path(), Identifier(libCheckRes->first, libCheckRes->second),
          Lexer->getContent(), std::move(parseRes), VisibilityInfo::pub(), ctx));
    } else {
      fileEntities.push_back(IR::QatModule::CreateFileMod(
          nullptr, fs::absolute(mainPath), mainPath.parent_path(), Identifier(mainPath.filename().string(), mainPath),
          Lexer->getContent(), std::move(parseRes), VisibilityInfo::pub(), ctx));
    }
  }
  for (const auto& bPath : broughtPaths) {
    handlePath(bPath, ctx);
  }
  broughtPaths.clear();
  for (const auto& mPath : memberPaths) {
    removeEntityWithPath(mPath);
  }
  memberPaths.clear();
}

bool QatSitter::checkExecutableExists(const String& name) {
#if PlatformIsWindows
  if (name.ends_with(".exe")) {
    if (system(("where " + name + " > nul").c_str())) {
      return false;
    }
    return true;
  } else {
    if (system(("where " + name + " > nul").c_str()) && system(("where " + name + ".exe > nul").c_str())) {
      return false;
    }
    return true;
  }
#else
  if (std::system(("which " + name + " > /dev/null").c_str())) {
    return false;
  } else {
    return true;
  }
#endif
}

void QatSitter::destroy() {
  IR::QatModule::clearAll();
  ast::Node::clearAll();
  ast::QatType::clearAll();
  IR::Value::clearAll();
  IR::QatType::clearAll();
}

QatSitter::~QatSitter() { destroy(); }

} // namespace qat
