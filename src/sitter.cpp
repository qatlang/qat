#include "./sitter.hpp"
#include "./IR/stdlib.hpp"
#include "./show.hpp"
#include "IR/qat_module.hpp"
#include "IR/value.hpp"
#include "ast/types/qat_type.hpp"
#include "cli/config.hpp"
#include "lexer/lexer.hpp"
#include "lexer/token_type.hpp"
#include "parser/parser.hpp"
#include "utils/identifier.hpp"
#include "utils/logger.hpp"
#include "utils/pstream/pstream.h"
#include "utils/qat_region.hpp"
#include "utils/visibility.hpp"
#include <chrono>
#include <filesystem>
#include <ios>
#include <system_error>
#include <thread>

#if PlatformIsWindows
#include "windows.h"
#endif

#define OUTPUT_OBJECT_NAME "output"

namespace qat {

QatSitter* QatSitter::instance = nullptr;

QatSitter::QatSitter()
    : ctx(IR::Context::New()), Lexer(lexer::Lexer::get(ctx)), Parser(parser::Parser::get(ctx)),
      mainThread(std::this_thread::get_id()) {
  ctx->sitter = this;
}

QatSitter* QatSitter::get() {
  if (instance) {
    return instance;
  }
  instance = std::construct_at(OwnTracked(QatSitter));
  return instance;
}

void QatSitter::display_stats() {
  auto& log = Logger::get();
  log->diagnostic(
      "Lexer speed   -> " +
      std::to_string(
          (u64)((((double)lexer::Lexer::lineCount) / ((double)lexer::Lexer::timeInMicroSeconds)) * 1000000.0)) +
      " lines/s");
  log->diagnostic(
      "Parser speed  -> " +
      std::to_string(
          (u64)((((double)lexer::Lexer::lineCount) / ((double)parser::Parser::timeInMicroSeconds)) * 1000000.0)) +
      " lines/s");
  if (ctx->clangAndLinkTimeInMs.has_value()) {
    log->diagnostic(
        "Compile speed -> " +
        std::to_string(
            (u64)((((double)lexer::Lexer::lineCount) / ((double)ctx->qatCompileTimeInMs.value())) * 1000000.0)) +
        " lines/s");
  }
  auto timeToString = [](u64 timeInMs) {
    if (timeInMs > 1000000) {
      return std::to_string(((double)timeInMs) / 1000000) + " s";
    } else if (timeInMs > 1000) {
      return std::to_string((double)timeInMs / 1000) + " ms";
    } else {
      return std::to_string(timeInMs) + " μs";
    }
  };
  log->diagnostic("Lexer time    -> " + timeToString(lexer::Lexer::timeInMicroSeconds));
  log->diagnostic("Parser time   -> " + timeToString(parser::Parser::timeInMicroSeconds));
  if (ctx->qatCompileTimeInMs.has_value() && ctx->clangAndLinkTimeInMs.has_value()) {
    log->diagnostic("Compile time  -> " + timeToString(ctx->qatCompileTimeInMs.value()));
    log->diagnostic("clang & lld   -> " + timeToString(ctx->clangAndLinkTimeInMs.value()));
  }
  log->finish_output();
}

void QatSitter::initialise() {
  auto* config = cli::Config::get();
  auto& log    = Logger::get();
  for (const auto& path : config->getPaths()) {
    SHOW("Handling path for " << path)
    handlePath(path, ctx);
  }
  if (config->hasStdLibPath() && ctx->stdLibRequired) {
    handlePath(config->getStdLibPath(), ctx);
    if (IR::QatModule::hasFileModule(config->getStdLibPath())) {
      IR::StdLib::stdLib = IR::QatModule::getFileModule(config->getStdLibPath());
    }
  }
  if (config->isCompile() || config->isAnalyse()) {
    auto qatStartTime = std::chrono::high_resolution_clock::now();
    for (auto* entity : fileEntities) {
      entity->handleFilesystemBrings(ctx);
    }
    for (auto* entity : fileEntities) {
      entity->createModules(ctx);
    }
    for (auto* entity : fileEntities) {
      SHOW("Create Entity: " << entity->getName())
      entity->create_entities(ctx);
    }
    for (auto* entity : fileEntities) {
      SHOW("Update Entity Dependencies: " << entity->getName())
      entity->update_entity_dependencies(ctx);
    }
    // for (auto* entity : fileEntities) {
    //   entity->handleBrings(ctx);
    // }
    // for (auto* entity : fileEntities) {
    //   entity->defineTypes(ctx);
    // }
    // for (auto* entity : fileEntities) {
    //   entity->defineNodes(ctx);
    // }
    // for (auto* entity : fileEntities) {
    //   entity->handleBrings(ctx);
    // }
    bool atleastOneEntityDone  = true;
    bool hasIncompleteEntities = true;
    while (hasIncompleteEntities && atleastOneEntityDone) {
      atleastOneEntityDone  = false;
      hasIncompleteEntities = false;
      for (auto* itMod : IR::QatModule::allModules) {
        auto oldMod = ctx->setActiveModule(itMod);
        for (auto ent : itMod->entityEntries) {
          SHOW("Entity " << (ent->name.has_value() ? ("hasName: " + ent->name.value().value) : ""))
          SHOW("      type: " << IR::entity_type_to_string(ent->type))
          SHOW("      Is complete: " << (ent->are_all_phases_complete() ? "true" : "false"))
          SHOW("      Is ready: " << (ent->is_ready_for_next_phase() ? "true" : "false"))
          SHOW("      Iterations: " << ent->iterations)
          if (!ent->are_all_phases_complete()) {
            if (ent->is_ready_for_next_phase()) {
              ent->do_next_phase(itMod, ctx);
              atleastOneEntityDone = true;
            }
            if (!ent->are_all_phases_complete()) {
              hasIncompleteEntities = true;
            }
          }
          ent->iterations++;
        }
        (void)ctx->setActiveModule(oldMod);
      }
    }
    if (!atleastOneEntityDone && hasIncompleteEntities) {
      Vec<IR::QatError> errors;
      for (auto* iterMod : IR::QatModule::allModules) {
        for (auto ent : iterMod->entityEntries) {
          if (!ent->are_all_phases_complete()) {
            String                     depStr;
            usize                      incompleteDepCount = 0;
            std::set<IR::EntityState*> ents;
            for (auto dep : ent->dependencies) {
              if (ents.contains(dep.entity)) {
                continue;
              }
              ents.insert(dep.entity);
              if (dep.entity->supportsChildren ? (dep.entity->status != IR::EntityStatus::childrenPartial)
                                               : (dep.entity->status != IR::EntityStatus::complete)) {
                if (dep.entity->name.has_value()) {
                  depStr += (dep.type == IR::DependType::partial ? "- depends partially on " : "- depends on ") +
                            ctx->highlightError(iterMod->getFullNameWithChild(dep.entity->name.value().value)) +
                            +" at " + dep.entity->name.value().range.startToString() + "\n";
                } else {
                  depStr += String(dep.type == IR::DependType::partial ? "- Depends partially on " : " - Depends on ") +
                            "unnamed " + IR::entity_type_to_string(ent->type) +
                            (dep.entity->astNode ? (" at " + dep.entity->astNode->fileRange.startToString()) : "") +
                            "\n";
                }
                incompleteDepCount++;
              }
            }
            errors.push_back(IR::QatError(
                "This " + String(ent->status == IR::EntityStatus::partial ? "partially created " : "") +
                    IR::entity_type_to_string(ent->type) + " " +
                    (ent->name.has_value() ? ctx->highlightError(ent->name.value().value) + " " : "") +
                    "could not be finalised as its dependencies were not resolved properly. This entity has " +
                    ctx->highlightError(std::to_string(incompleteDepCount)) + " incomplete dependenc" +
                    (incompleteDepCount > 1 ? "ies" : "y") +
                    ((incompleteDepCount > 0) ? ((incompleteDepCount > 1 ? ". The dependencies are\n" : "\n") + depStr)
                                              : ""),
                ent->astNode ? ent->astNode->fileRange
                             : (ent->name.has_value() ? Maybe<FileRange>(ent->name.value().range) : None)));
          }
        }
      }
      ctx->Errors(errors);
    }
    for (auto* entity : fileEntities) {
      entity->setup_llvm_file(ctx);
    }
    // for (auto* entity : fileEntities) {
    //   auto* oldMod = ctx->setActiveModule(entity);
    //   entity->emitNodes(ctx);
    //   (void)ctx->setActiveModule(oldMod);
    // }
    SHOW("Emitted nodes")
    auto qatCompileTime =
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - qatStartTime)
            .count();
    ctx->qatCompileTimeInMs = qatCompileTime;
    auto* cfg               = cli::Config::get();
    // if (cfg->hasOutputPath()) {
    //   fs::remove_all(cfg->getOutputPath() / "llvm");
    // }

    //
    //
    if (cfg->exportCodeMetadata()) {
      log->say("Exporting code metadata");
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
    auto clear_llvm_files = [&] {
      if (cfg->clearLLVM()) {
        for (const auto& llPath : ctx->llvmOutputPaths) {
          fs::remove(llPath);
        }
        if (cfg->hasOutputPath() && fs::exists(cfg->getOutputPath() / "llvm")) {
          fs::remove_all(cfg->getOutputPath() / "llvm");
        }
      }
    };
    auto clangStartTime = std::chrono::high_resolution_clock::now();
    if (cfg->isCompile()) {
      SHOW("Checking whether clang exists or not")
      if (checkExecutableExists("clang") || checkExecutableExists("clang++")) {
        for (auto* entity : fileEntities) {
          entity->compileToObject(ctx);
        }
        ctx->finalise_errors();
        IR::QatModule::find_native_library_paths();
        for (auto* entity : fileEntities) {
          entity->handleNativeLibs(ctx);
        }
        ctx->finalise_errors();
        for (auto* entity : fileEntities) {
          entity->bundleLibs(ctx);
        }
        ctx->clangAndLinkTimeInMs = std::chrono::duration_cast<std::chrono::microseconds>(
                                        std::chrono::high_resolution_clock::now() - clangStartTime)
                                        .count();
        ctx->finalise_errors();
        display_stats();
        ctx->writeJsonResult(true);
        clear_llvm_files();
        if (cfg->isRun() && !ctx->executablePaths.empty()) {
          for (const auto& exePath : ctx->executablePaths) {
            SHOW("Running built executable at: " << exePath.string())
            redi::ipstream proc(exePath.string(), redi::pstreams::pstdout);
            String         output;
            String         errOut;
            String         line;
            while (std::getline(proc.out(), line)) {
              output += line + "\n";
            }
            while (std::getline(proc.err(), line)) {
              errOut += line + "\n";
            }
            while (!proc.rdbuf()->exited()) {
            }
            log->out << "\n===== Output of \"" + exePath.lexically_relative(fs::current_path()).string() + "\"\n" +
                            output + "===== Status Code: " + std::to_string(proc.rdbuf()->status()) + "\n";
            if (proc.rdbuf()->status()) {
              ctx->Error("\nThe built executable at " + ctx->highlightError(exePath.string()) + " exited with error\n" +
                             errOut,
                         None);
            }
            proc.close();
          }
          SHOW("Ran all compiled executables")
        }
      } else {
        ctx->Error("qat cannot find clang on path. Please make sure that you have clang installed and the "
                   "path to clang is available in the environment",
                   None);
      }
    } else {
      display_stats();
      clear_llvm_files();
      SHOW("clearLLVM called")
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
          Parser->set_tokens(Lexer->getTokens());
          auto parseRes(Parser->parse());
          for (const auto& bPath : Parser->get_brought_paths()) {
            broughtPaths.push_back(bPath);
          }
          for (const auto& mPath : Parser->get_member_paths()) {
            memberPaths.push_back(mPath);
          }
          Parser->clear_brought_paths();
          Parser->clear_member_paths();
          fileEntities.push_back(IR::QatModule::CreateRootLib(parentMod, fs::absolute(libCheckRes->second), path,
                                                              Identifier(libCheckRes->first, libCheckRes->second),
                                                              std::move(parseRes), VisibilityInfo::pub(), ctx));
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
        Parser->set_tokens(Lexer->getTokens());
        auto parseRes(Parser->parse());
        for (const auto& bPath : Parser->get_brought_paths()) {
          broughtPaths.push_back(bPath);
        }
        for (const auto& mPath : Parser->get_member_paths()) {
          memberPaths.push_back(mPath);
        }
        Parser->clear_brought_paths();
        Parser->clear_member_paths();
        if (libCheckRes.has_value()) {
          fileEntities.push_back(IR::QatModule::CreateRootLib(parentMod, fs::absolute(item), path,
                                                              Identifier(libCheckRes->first, libCheckRes->second),
                                                              std::move(parseRes), VisibilityInfo::pub(), ctx));
        } else {
          fileEntities.push_back(IR::QatModule::CreateFileMod(parentMod, fs::absolute(item), path,
                                                              Identifier(item.path().filename().string(), item.path()),
                                                              std::move(parseRes), VisibilityInfo::pub(), ctx));
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
      Parser->set_tokens(Lexer->getTokens());
      auto parseRes(Parser->parse());
      for (const auto& bPath : Parser->get_brought_paths()) {
        broughtPaths.push_back(bPath);
      }
      for (const auto& mPath : Parser->get_member_paths()) {
        memberPaths.push_back(mPath);
      }
      Parser->clear_brought_paths();
      Parser->clear_member_paths();
      fileEntities.push_back(IR::QatModule::CreateFileMod(nullptr, libCheckRes->second, mainPath,
                                                          Identifier(libCheckRes->first, libCheckRes->second),
                                                          std::move(parseRes), VisibilityInfo::pub(), ctx));
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
    Parser->set_tokens(Lexer->getTokens());
    auto parseRes(Parser->parse());
    for (const auto& bPath : Parser->get_brought_paths()) {
      broughtPaths.push_back(bPath);
    }
    for (const auto& mPath : Parser->get_member_paths()) {
      memberPaths.push_back(mPath);
    }
    Parser->clear_brought_paths();
    Parser->clear_member_paths();
    if (libCheckRes.has_value()) {
      fileEntities.push_back(IR::QatModule::CreateRootLib(nullptr, fs::absolute(mainPath), mainPath.parent_path(),
                                                          Identifier(libCheckRes->first, libCheckRes->second),
                                                          std::move(parseRes), VisibilityInfo::pub(), ctx));
    } else {
      fileEntities.push_back(IR::QatModule::CreateFileMod(nullptr, fs::absolute(mainPath), mainPath.parent_path(),
                                                          Identifier(mainPath.filename().string(), mainPath),
                                                          std::move(parseRes), VisibilityInfo::pub(), ctx));
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
  IR::Value::replace_uses_for_all();
  IR::QatModule::clearAll();
  ast::Node::clearAll();
  ast::QatType::clearAll();
  IR::Value::clearAll();
  IR::QatType::clearAll();
}

QatSitter::~QatSitter() { destroy(); }

} // namespace qat
