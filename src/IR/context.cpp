#include "./context.hpp"
#include "../ast/node.hpp"
#include "../cli/color.hpp"
#include "../cli/config.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../qat_sitter.hpp"
#include "../utils/logger.hpp"
#include "../utils/qat_region.hpp"
#include "./value.hpp"
#include "fstream"
#include "member_function.hpp"
#include "qat_module.hpp"
#include "clang/Basic/AddressSpaces.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/LLVMContext.h"
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>

namespace qat::IR {

#define Colored(val) (cfg->noColorMode() ? "" : val)

#define ColoredOr(val, rep) (cfg->noColorMode() ? rep : val)

LoopInfo::LoopInfo(String _name, IR::Block* _mainB, IR::Block* _condB, IR::Block* _restB, IR::LocalValue* _index,
                   LoopType _type)
    : name(std::move(_name)), mainBlock(_mainB), condBlock(_condB), restBlock(_restB), index(_index), type(_type) {}

Breakable::Breakable(Maybe<String> _tag, IR::Block* _restBlock, IR::Block* _trueBlock)
    : tag(std::move(_tag)), restBlock(_restBlock), trueBlock(_trueBlock) {}

CodeProblem::CodeProblem(bool _isError, String _message, Maybe<FileRange> _range)
    : isError(_isError), message(std::move(_message)), range(std::move(_range)) {}

CodeProblem::operator Json() const {
  return Json()
      ._("isError", isError)
      ._("message", message)
      ._("hasRange", range.has_value())
      ._("fileRange", range.has_value() ? (Json)(range.value()) : Json());
}

QatError::QatError() = default;

QatError::QatError(String _message, Maybe<FileRange> _range) : message(_message), fileRange(_range) {}

QatError& QatError::add(String value) {
  message.append(value);
  return *this;
}

QatError& QatError::colored(String value, const char* color) {
  auto* cfg = cli::Config::get();
  message.append(ColoredOr(color, "`") + message + ColoredOr(colors::bold::white, "`"));
  return *this;
}

void QatError::setRange(FileRange range) { fileRange = range; }

Context* Context::instance = nullptr;

Context::Context() : llctx(), clangTargetInfo(nullptr), builder(llctx), hasMain(false) {
  SHOW("llctx address: " << &llctx)
  SHOW("Builder llctx address: " << &builder.getContext())
  auto diagnosticEngine =
      clang::DiagnosticsEngine(llvm::IntrusiveRefCntPtr<clang::DiagnosticIDs>(new clang::DiagnosticIDs()),
                               llvm::IntrusiveRefCntPtr<clang::DiagnosticOptions>(new clang::DiagnosticOptions()));
  auto targetOpts    = std::make_shared<clang::TargetOptions>();
  targetOpts->Triple = cli::Config::get()->getTargetTriple();
  clangTargetInfo    = clang::TargetInfo::CreateTargetInfo(diagnosticEngine, targetOpts);
  dataLayout         = llvm::DataLayout(clangTargetInfo->getDataLayoutString());
}

void Context::nameCheckInModule(const Identifier& name, const String& entityType, Maybe<String> genericID,
                                Maybe<String> opaqueID) {
  auto reqInfo = getAccessInfo();
  if (getActiveModule()->hasOpaqueType(name.value, reqInfo)) {
    auto* opq = getActiveModule()->getOpaqueType(name.value, getAccessInfo());
    if (opq->isGeneric()) {
      if (genericID.has_value()) {
        if (opq->getGenericID().value() == genericID.value()) {
          return;
        }
      }
    } else if (opaqueID.has_value()) {
      if (opq->getID() == opaqueID.value()) {
        return;
      }
    }
    String tyDesc;
    if (opq->isSubtypeCore()) {
      tyDesc = opq->isGeneric() ? "generic core type" : "core type";
    } else if (opq->isSubtypeMix()) {
      tyDesc = opq->isGeneric() ? "generic mix type" : "mix type";
    } else {
      tyDesc = opq->isGeneric() ? "generic type" : "type";
    }
    Error("A " + tyDesc + " named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasCoreType(name.value, reqInfo)) {
    Error("A core type named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtCoreType(name.value, None)) {
    Error("A core type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleCoreTypeInImports(name.value, reqInfo).first) {
    Error("A core type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleCoreTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasGenericCoreType(name.value, reqInfo)) {
    if (genericID.has_value() &&
        getActiveModule()->getGenericCoreType(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic core type named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtGenericCoreType(name.value, None)) {
    if (genericID.has_value() &&
        getActiveModule()->getGenericCoreType(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic core type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleGenericCoreTypeInImports(name.value, reqInfo).first) {
    Error("A generic core type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleGenericCoreTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasMixType(name.value, reqInfo)) {
    Error("A mix type named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtMixType(name.value, None)) {
    Error("A mix type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleMixTypeInImports(name.value, reqInfo).first) {
    Error("A mix type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleMixTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasChoiceType(name.value, reqInfo)) {
    Error("A choice type named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtChoiceType(name.value, None)) {
    Error("A choice type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleChoiceTypeInImports(name.value, reqInfo).first) {
    Error("A choice type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleChoiceTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasTypeDef(name.value, reqInfo)) {
    Error("A type definition named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtTypeDef(name.value, None)) {
    Error("A type definition named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleTypeDefInImports(name.value, reqInfo).first) {
    Error("A type definition named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleTypeDefInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasFunction(name.value, reqInfo)) {
    Error("A function named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtFunction(name.value, None)) {
    Error("A function named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleFunctionInImports(name.value, reqInfo).first) {
    Error("A function named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleFunctionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasGenericFunction(name.value, reqInfo)) {
    if (genericID.has_value() &&
        getActiveModule()->getGenericFunction(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtGenericFunction(name.value, None)) {
    if (genericID.has_value() &&
        getActiveModule()->getGenericFunction(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleGenericFunctionInImports(name.value, reqInfo).first) {
    if (genericID.has_value() &&
        getActiveModule()->getGenericFunction(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleGenericFunctionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasGlobalEntity(name.value, reqInfo)) {
    Error("A global entity named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtGlobalEntity(name.value, None)) {
    Error("A global entity named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleGlobalEntityInImports(name.value, reqInfo).first) {
    Error("A global entity named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleGlobalEntityInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasPrerunGlobal(name.value, reqInfo)) {
    Error("A prerun global entity named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtPrerunGlobal(name.value, None)) {
    Error("A prerun global entity named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessiblePrerunGlobalInImports(name.value, reqInfo).first) {
    Error("A prerun global entity named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleGlobalEntityInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasRegion(name.value, reqInfo)) {
    Error("A region named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtRegion(name.value, None)) {
    Error("A region named " + highlightError(name.value) + " is brought into this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleRegionInImports(name.value, reqInfo).first) {
    Error("A region named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleRegionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBox(name.value, reqInfo)) {
    Error("A box named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtBox(name.value, None)) {
    Error("A box named " + highlightError(name.value) + " is brought into this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleBoxInImports(name.value, reqInfo).first) {
    Error("A box named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleRegionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasLib(name.value, reqInfo)) {
    Error("A lib named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasBroughtLib(name.value, None)) {
    Error("A lib named " + highlightError(name.value) + " is brought into this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (getActiveModule()->hasAccessibleLibInImports(name.value, reqInfo).first) {
    Error("A lib named " + highlightError(name.value) + " is present inside the module " +
              highlightError(getActiveModule()->hasAccessibleRegionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  }
}

llvm::GlobalValue::LinkageTypes Context::getGlobalLinkageForVisibility(VisibilityInfo const& visibInfo) const {
  switch (visibInfo.kind) {
    case VisibilityKind::pub:
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    case VisibilityKind::type:
      return llvm::GlobalValue::LinkageTypes::InternalLinkage;
    case VisibilityKind::folder:
    case VisibilityKind::file:
    case VisibilityKind::box:
    case VisibilityKind::lib: {
      if (visibInfo.moduleVal) {
        if (!visibInfo.moduleVal->hasSubmodules()) {
          return llvm::GlobalValue::LinkageTypes::InternalLinkage;
        }
      }
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    }
    case VisibilityKind::parent: {
      if (visibInfo.typePtr) {
        return llvm::GlobalValue::LinkageTypes::InternalLinkage;
      } else if (visibInfo.moduleVal) {
        if (!visibInfo.moduleVal->hasSubmodules()) {
          return llvm::GlobalValue::LinkageTypes::InternalLinkage;
        }
      }
      return llvm::GlobalValue::LinkageTypes::ExternalLinkage;
    }
  }
}

VisibilityInfo Context::getVisibInfo(Maybe<ast::VisibilitySpec> spec) {
  if (spec.has_value() && (spec.value().kind != VisibilityKind::parent)) {
    SHOW("Visibility kind has value")
    switch (spec->kind) {
      case VisibilityKind::box: {
        if (getMod()->hasClosestParentBox()) {
          return VisibilityInfo::box(getMod()->getClosestParentBox());
        } else {
          Error("The current module does not have a parent box", spec->range);
        }
      }
      case VisibilityKind::lib: {
        if (getMod()->hasClosestParentLib()) {
          return VisibilityInfo::lib(getMod()->getClosestParentLib());
        } else {
          Error("The current module does not have a parent lib", spec->range);
        }
      }
      case VisibilityKind::file: {
        return VisibilityInfo::file(getMod()->getParentFile());
      }
      case VisibilityKind::folder: {
        auto folderPath = fs::canonical(fs::path(getMod()->getFilePath()).parent_path());
        if (!getMod()->hasFolderModule(folderPath)) {
          Error("Could not find folder module with path: " + highlightError(folderPath.string()), spec->range);
        }
        return VisibilityInfo::folder(getMod()->getFolderModule(folderPath));
      }
      case VisibilityKind::skill: {
        if (hasActiveType()) {
          return VisibilityInfo::skill(getActiveType());
        } else {
          Error("There is no parent type in the scope and hence this visibility is not allowed", spec->range);
        }
      }
      case VisibilityKind::type: {
        if (hasActiveType()) {
          return VisibilityInfo::type(getActiveType());
        } else {
          if (hasActiveFunction() && getActiveFunction()->isMemberFunction()) {
            return VisibilityInfo::type(((IR::MemberFunction*)getActiveFunction())->getParentType());
          } else if (has_active_done_skill()) {
            return VisibilityInfo::type(get_active_done_skill()->getType());
          } else {
            Error("There is no parent type and hence " + highlightError("type") + " visibility cannot be used here",
                  spec->range);
          }
        }
      }
      case VisibilityKind::pub: {
        return VisibilityInfo::pub();
      }
      default:
        break;
    }
  } else {
    SHOW("No visibility kind")
    if (hasActiveType()) {
      SHOW("Found active type")
      return VisibilityInfo::type(getActiveType());
    } else if (has_active_skill() || has_active_done_skill()) {
      return VisibilityInfo::pub();
    } else {
      SHOW("No active type")
      switch (getMod()->getModuleType()) {
        case ModuleType::box: {
          return VisibilityInfo::box(getMod());
        }
        case ModuleType::file: {
          return VisibilityInfo::file(getMod());
        }
        case ModuleType::lib: {
          return VisibilityInfo::lib(getMod());
        }
        case ModuleType::folder: {
          return VisibilityInfo::folder(getMod());
        }
      }
    }
  }
  SHOW("No visibility info found")
} // NOLINT(clang-diagnostic-return-type)

AccessInfo Context::getAccessInfo() const {
  IR::QatModule*        mod   = getActiveModule();
  Maybe<IR::QatType*>   type  = None;
  Maybe<IR::DoneSkill*> skill = None;
  if (has_active_done_skill()) {
    skill = get_active_done_skill();
  } else if (hasActiveType()) {
    type = getActiveType();
  }
  return {mod, type, skill};
}

String Context::highlightError(const String& message, const char* color) {
  auto* cfg = cli::Config::get();
  return ColoredOr(color, "`") + message + ColoredOr(colors::bold::white, "`");
}

String Context::highlightWarning(const String& message, const char* color) {
  auto* cfg = cli::Config::get();
  return ColoredOr(color, "`") + message + ColoredOr(colors::bold::white, "`");
}

void Context::writeJsonResult(bool status) const {
  SHOW("Pushing problems")
  Vec<JsonValue> problems;
  for (auto& prob : codeProblems) {
    SHOW("Pushing code problem")
    problems.push_back((Json)prob);
  }
  SHOW("Pushed all code problems")
  Json result;
  SHOW("Setting binary sizes in result")
  Vec<JsonValue> binarySizesJson;
  for (auto binSiz : binarySizes) {
    binarySizesJson.push_back(binSiz);
  }
  SHOW("Creating JSON object for result")
  result._("status", status)
      ._("problems", problems)
      ._("lineCount", lexer::Lexer::lineCount > 0 ? lexer::Lexer::lineCount - 1 : 0)
      ._("lexerTime", lexer::Lexer::timeInMicroSeconds)
      ._("parserTime", parser::Parser::timeInMicroSeconds)
      ._("compilationTime", qatCompileTimeInMs.has_value() ? qatCompileTimeInMs.value() : JsonValue())
      ._("linkingTime", clangAndLinkTimeInMs.has_value() ? clangAndLinkTimeInMs.value() : JsonValue())
      ._("binarySizes", binarySizesJson)
      ._("hasMain", hasMain);
  SHOW("Creating compilation result file")
  std::ofstream output;
  auto          outPath = cli::Config::get()->getOutputPath() / "QatCompilationResult.json";
  if (fs::exists(outPath)) {
    fs::remove(outPath);
  }
  output.open(outPath, std::ios_base::out);
  if (output.is_open()) {
    output << result;
    output.close();
  }
  SHOW("compilation result file done")
}

bool Context::hasGenericParameterFromLastMain(String const& name) const {
  if (lastMainActiveGeneric.empty()) {
    return allActiveGenerics.back().hasGenericParameter(name);
  } else {
    for (auto i = lastMainActiveGeneric.back(); i < allActiveGenerics.size(); i++) {
      if (allActiveGenerics.at(i).hasGenericParameter(name)) {
        return true;
      }
    }
    return false;
  }
}

GenericParameter* Context::getGenericParameterFromLastMain(String const& name) const {
  if (lastMainActiveGeneric.empty()) {
    if (allActiveGenerics.back().hasGenericParameter(name)) {
      return allActiveGenerics.back().getGenericParameter(name);
    }
  } else {
    for (auto i = lastMainActiveGeneric.back(); i < allActiveGenerics.size(); i++) {
      if (allActiveGenerics.at(i).hasGenericParameter(name)) {
        return allActiveGenerics.at(i).getGenericParameter(name);
      }
    }
  }
  return nullptr;
}

void Context::addError(String const& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo) {
  while (!ctxMut.try_lock()) {
  }
  threadsWithErrors.insert(std::this_thread::get_id());
  auto* cfg = cli::Config::get();
  if (hasActiveGeneric()) {
    codeProblems.push_back(CodeProblem(true,
                                       "Errors generated while creating generic variant: " + getActiveGeneric().name,
                                       getActiveGeneric().fileRange));
    Logger::get()->errOut << "\n"
                          << Colored(colors::highIntensityBackground::red) << " ERROR " << Colored(colors::cyan)
                          << " --> " << Colored(colors::reset) << getActiveGeneric().fileRange.file.string() << ":"
                          << getActiveGeneric().fileRange.start << "\n"
                          << "Errors while creating generic variant: " << highlightError(getActiveGeneric().name)
                          << "\n"
                          << "\n";
  }
  codeProblems.push_back(CodeProblem(
      true, (hasActiveGeneric() ? ("Creating " + getActiveGeneric().name + " => ") : "") + message, fileRange));
  Logger::get()->errOut << "\n"
                        << Colored(colors::highIntensityBackground::red) << " ERROR " << Colored(colors::reset) << " ";
  Logger::get()->errOut << Colored(colors::bold::white)
                        << (hasActiveGeneric() ? ("Creating " + getActiveGeneric().name + " => ") : "") << message
                        << Colored(colors::reset) << "\n";
  if (fileRange) {
    Logger::get()->errOut << Colored(colors::cyan) << " --> " << Colored(colors::reset)
                          << fileRange.value().file.string() << ":" << fileRange.value().start << " to "
                          << fileRange.value().end;
    print_range_content(fileRange.value(), true, true);
  }
  if (pointTo.has_value()) {
    Logger::get()->errOut << (fileRange.has_value() ? "" : "\n") << Colored(colors::bold::white)
                          << pointTo.value().first << Colored(colors::reset) << "\n"
                          << Colored(colors::cyan) << " --> " << Colored(colors::reset)
                          << pointTo.value().second.file.string() << ":" << pointTo.value().second.start << " to "
                          << pointTo.value().second.end;
    print_range_content(pointTo.value().second, true, false);
  }
  Logger::get()->errOut << "\n";
  if (hasActiveModule() && !module_has_errors(getActiveModule())) {
    if (hasActiveGeneric()) {
      removeActiveGeneric();
    }
    modulesWithErrors.push_back(getActiveModule());
    for (const auto& modNRange : getActiveModule()->getBroughtMentions()) {
      auto* oldMod = setActiveModule(modNRange.first);
      addError("Error occured in this file", modNRange.second);
      (void)setActiveModule(oldMod);
    }
  }
  Logger::get()->finish_output();
  ctxMut.unlock();
}

void Context::print_range_content(FileRange const& fileRange, bool isError, bool isContentError) const {
  auto cfg = cli::Config::get();
  if (!fs::is_regular_file(fileRange.file)) {
    return;
  }
  auto  lines       = get_range_content(fileRange);
  auto  endLine     = lines.first + lines.second.size();
  usize lineNumSize = std::to_string(lines.first).size();
  if (lineNumSize < std::to_string(endLine).size()) {
    lineNumSize = std::to_string(endLine).size();
  }
  usize lineIndex = 0;
  (isError ? Logger::get()->errOut : Logger::get()->out) << "\n";
  for (auto& lineInfo : lines.second) {
    String lineNum = std::to_string(lines.first + lineIndex);
    while (lineNum.size() < lineNumSize) {
      lineNum += " ";
    }
    (isError ? Logger::get()->errOut : Logger::get()->out) << lineNum << " | " << std::get<0>(lineInfo) << "\n";
    if (std::get<1>(lineInfo) != std::get<2>(lineInfo)) {
      String spacing;
      if (std::get<1>(lineInfo) > 0) {
        spacing.reserve(std::get<1>(lineInfo));
      }
      for (usize i = 0; i < std::get<1>(lineInfo); i++) {
        if (std::get<0>(lineInfo).at(i) == '\t') {
          spacing += "\t";
        } else {
          spacing += " ";
        }
      }
      auto   indicatorCount = std::get<2>(lineInfo) - std::get<1>(lineInfo);
      String indicator(indicatorCount, '^');
      (isError ? Logger::get()->errOut : Logger::get()->out)
          << String(lineNumSize, ' ') << " | " << spacing
          << (isContentError ? Colored(colors::bold::red) : Colored(colors::bold::purple)) << indicator
          << Colored(colors::reset) << "\n";
    }
    lineIndex++;
  }
}

void Context::add_exe_path(fs::path path) {
  while (!ctxMut.try_lock()) {
  }
  executablePaths.push_back(path);
  ctxMut.unlock();
}

void Context::add_binary_size(usize size) {
  while (!ctxMut.try_lock()) {
  }
  binarySizes.push_back(size);
  ctxMut.unlock();
}

void Context::finalise_errors() {
  while (!ctxMut.try_lock()) {
  }
  if (!threadsWithErrors.empty() && (sitter->mainThread == std::this_thread::get_id())) {
    Logger::get()->out.emit();
    Logger::get()->errOut.emit();
    writeJsonResult(false);
    sitter->destroy();
    QatRegion::destroyAllBlocks();
    ctxMut.unlock();
    std::exit(0);
  } else if (!threadsWithErrors.empty()) {
    ctxMut.unlock();
    pthread_exit(nullptr);
  }
  ctxMut.unlock();
}

void Context::Error(const String& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo) {
  addError(message, fileRange, pointTo);
  finalise_errors();
}

void Context::Errors(Vec<QatError> errors) {
  for (auto& err : errors) {
    addError(err.message, err.fileRange);
  }
  finalise_errors();
}

Pair<usize, Vec<std::tuple<String, u64, u64>>> Context::get_range_content(FileRange const& _range) const {
  Vec<std::tuple<String, u64, u64>> result;

  std::ifstream file(_range.file);
  String        line;
  u64           lineCount = 0;
  const usize   startLine = _range.start.line;
  const usize   startChar = _range.start.character;
  const usize   endLine   = _range.end.line;
  const usize   endChar   = _range.end.character;
  usize         firstLine = startLine;
  while (std::getline(file, line)) {
    lineCount++;
    usize i = 0;
    while (line[i] == '\t') {
      i++;
    }
    if ((startLine > 0u) && (lineCount == (startLine - 1))) {
      // Line before the first relevant line in file range
      result.push_back({line, 0u, 0u});
      firstLine = lineCount;
    } else if ((lineCount >= startLine) && (lineCount <= endLine)) {
      // Relevant line
      if (startLine == endLine) {
        // Only one line for the relevant content
        result.push_back({line, startChar, endChar});
      } else if (lineCount == startLine) {
        // First relevant line
        result.push_back({line, startChar, line.size()});
      } else if (lineCount == endLine) {
        // Last relevant line
        result.push_back({line, i, endChar});
      } else {
        // Relevant lines in the middle
        result.push_back({line, i, line.size()});
      }
    } else if ((endLine < UINT64_MAX) && (lineCount == (endLine + 1))) {
      // Line after the last relevant line in file range
      result.push_back({line, 0u, 0u});
      break;
    }
  }
  return {firstLine, result};
}

void Context::Warning(const String& message, const FileRange& fileRange) {
  while (!ctxMut.try_lock()) {
  }
  if (hasActiveGeneric()) {
    getActiveGeneric().warningCount++;
  }
  codeProblems.push_back(CodeProblem(
      false, (hasActiveGeneric() ? ("Creating " + joinActiveGenericNames(false) + " => ") : "") + message, fileRange));
  auto* cfg = cli::Config::get();
  Logger::get()->out << "\n"
                     << Colored(colors::highIntensityBackground::purple) << " WARNING " << Colored(colors::cyan)
                     << " --> " << Colored(colors::reset) << fileRange.file.string() << ":" << fileRange.start.line
                     << ":" << fileRange.start.character << Colored(colors::bold::white) << "\n"
                     << (hasActiveGeneric() ? ("Creating " + joinActiveGenericNames(true) + " => ") : "") << message
                     << Colored(colors::reset) << "\n";
  print_range_content(fileRange, false, false);
  ctxMut.unlock();
}

} // namespace qat::IR
