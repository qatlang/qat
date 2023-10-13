#include "./context.hpp"
#include "../ast/node.hpp"
#include "../cli/color.hpp"
#include "../cli/config.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "../qat_sitter.hpp"
#include "./value.hpp"
#include "fstream"
#include "member_function.hpp"
#include "clang/Basic/AddressSpaces.h"
#include "clang/Basic/DiagnosticDriver.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Target/TargetOptions.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>

namespace qat::IR {

#define Colored(val) (cfg->noColorMode() ? "" : val)

#define ColoredOr(val, rep) (cfg->noColorMode() ? rep : val)

LoopInfo::LoopInfo(String _name, IR::Block* _mainB, IR::Block* _condB, IR::Block* _restB, IR::LocalValue* _index,
                   LoopType _type)
    : name(std::move(_name)), mainBlock(_mainB), condBlock(_condB), restBlock(_restB), index(_index), type(_type) {}

bool LoopInfo::isTimes() const { return type == LoopType::nTimes; }

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

Context* Context::instance = nullptr;

Context* Context::New() {
  if (instance) {
    return instance;
  } else {
    instance = new Context();
    return instance;
  }
}

llvm::LLVMContext& Context::getllCtx() {
  std::cout << "llctx(" << &llctx << ")\n";
  return (this->llctx);
}

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

void Context::genericNameCheck(const String& name, const FileRange& range) {
  if (fn && fn->hasGenericParameter(name)) {
    Error("A generic parameter named " + highlightError(name) +
              " is present in this function. This will lead to ambiguity.",
          range);
  } else if (activeType && activeType->hasGenericParameter(name)) {
    Error("A generic parameter named " + highlightError(name) + " is present in the parent type " +
              highlightError(activeType->getFullName()) + " so this will lead to ambiguity",
          range);
  }
}

void Context::nameCheckInModule(const Identifier& name, const String& entityType, Maybe<String> genericID) {
  auto reqInfo = getAccessInfo();
  if (mod->hasCoreType(name.value)) {
    Error("A core type named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtCoreType(name.value, None)) {
    Error("A core type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).first) {
    Error("A core type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleCoreTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasGenericCoreType(name.value)) {
    if (genericID.has_value() && mod->getGenericCoreType(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic core type named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtGenericCoreType(name.value, None)) {
    if (genericID.has_value() && mod->getGenericCoreType(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic core type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleGenericCoreTypeInImports(name.value, reqInfo).first) {
    Error("A generic core type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleGenericCoreTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasMixType(name.value)) {
    Error("A mix type named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtMixType(name.value, None)) {
    Error("A mix type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleMixTypeInImports(name.value, reqInfo).first) {
    Error("A mix type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleMixTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasChoiceType(name.value)) {
    Error("A choice type named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtChoiceType(name.value, None)) {
    Error("A choice type named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleChoiceTypeInImports(name.value, reqInfo).first) {
    Error("A choice type named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleChoiceTypeInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasTypeDef(name.value)) {
    Error("A type definition named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtTypeDef(name.value, None)) {
    Error("A type definition named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleTypeDefInImports(name.value, reqInfo).first) {
    Error("A type definition named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleTypeDefInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasFunction(name.value)) {
    Error("A function named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtFunction(name.value, None)) {
    Error("A function named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleFunctionInImports(name.value, reqInfo).first) {
    Error("A function named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleFunctionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasGenericFunction(name.value)) {
    if (genericID.has_value() && mod->getGenericFunction(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtGenericFunction(name.value, None)) {
    if (genericID.has_value() && mod->getGenericFunction(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleGenericFunctionInImports(name.value, reqInfo).first) {
    if (genericID.has_value() && mod->getGenericFunction(name.value, getAccessInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleGenericFunctionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasGlobalEntity(name.value)) {
    Error("A global entity named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtGlobalEntity(name.value, None)) {
    Error("A global entity named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleGlobalEntityInImports(name.value, reqInfo).first) {
    Error("A global entity named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleGlobalEntityInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasRegion(name.value)) {
    Error("A region named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtRegion(name.value, None)) {
    Error("A region named " + highlightError(name.value) + " is brought into this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleRegionInImports(name.value, reqInfo).first) {
    Error("A region named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleRegionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBox(name.value)) {
    Error("A box named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtBox(name.value, None)) {
    Error("A box named " + highlightError(name.value) + " is brought into this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleBoxInImports(name.value, reqInfo).first) {
    Error("A box named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleRegionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasLib(name.value)) {
    Error("A lib named " + highlightError(name.value) + " exists in this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtLib(name.value, None)) {
    Error("A lib named " + highlightError(name.value) + " is brought into this module. Please change name of this " +
              entityType + " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleLibInImports(name.value, reqInfo).first) {
    Error("A lib named " + highlightError(name.value) + " is present inside the module " +
              highlightError(mod->hasAccessibleRegionInImports(name.value, reqInfo).second) +
              " which is brought into the current module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  }
}

QatModule* Context::getMod() const { return mod->getActive(); }

String Context::getGlobalStringName() const {
  auto res = "qat'str'" + std::to_string(stringCount);
  stringCount++;
  return res;
}

VisibilityInfo Context::getVisibInfo(Maybe<VisibilityKind> kind) const {
  if (kind.has_value() && (kind.value() != VisibilityKind::parent)) {
    SHOW("Visibility kind has value")
    switch (kind.value()) {
      case VisibilityKind::box: {
        return VisibilityInfo::box(getMod()->getClosestParentBox()->getFullName());
      }
      case VisibilityKind::lib: {
        return VisibilityInfo::lib(getMod()->getClosestParentLib()->getFullName());
      }
      case VisibilityKind::file: {
        return VisibilityInfo::file(getMod()->getFilePath());
      }
      case VisibilityKind::folder: {
        return VisibilityInfo::folder(fs::path(getMod()->getFilePath()).parent_path().string());
      }
      case VisibilityKind::type: {
        if (activeType) {
          return VisibilityInfo::type(activeType->getFullName());
        } else {
          if (fn && fn->isMemberFunction()) {
            return VisibilityInfo::type(((IR::MemberFunction*)fn)->getParentType()->getFullName());
          } else {
            return VisibilityInfo::type("");
          }
        }
        // TODO - Handle in case it is null
      }
      case VisibilityKind::pub: {
        return VisibilityInfo::pub();
      }
      default:
        break;
    }
  } else {
    SHOW("No visibility kind")
    if (activeType) {
      SHOW("Found active type")
      return VisibilityInfo::type(activeType->getFullName());
    } else {
      SHOW("No active type")
      switch (getMod()->getModuleType()) {
        case ModuleType::box: {
          return VisibilityInfo::box(getMod()->getFullName());
        }
        case ModuleType::file: {
          return VisibilityInfo::file(getMod()->getFilePath());
        }
        case ModuleType::lib: {
          return VisibilityInfo::lib(getMod()->getFullName());
        }
        case ModuleType::folder: {
          return VisibilityInfo::folder(fs::path(getMod()->getParentFile()->getFilePath()).parent_path().string());
        }
      }
    }
  }
  SHOW("No visibility info found")
} // NOLINT(clang-diagnostic-return-type)

AccessInfo Context::getAccessInfo() const {
  // TODO - Consider changing string value to pointer of the actual entities
  Maybe<String> lib  = None;
  Maybe<String> box  = None;
  Maybe<String> type = None;
  String        file;
  if (mod) {
    file = mod->getParentFile()->getFilePath();
    if (mod->hasClosestParentBox()) {
      box = mod->getClosestParentBox()->getFullName();
    }
    if (mod->hasClosestParentLib()) {
      lib = mod->getClosestParentLib()->getFullName();
    }
  }
  if (activeType) {
    type = activeType->getFullName();
  } else if (fn) {
    if (fn->isMemberFunction()) {
      type = ((MemberFunction*)fn)->getParentType()->getFullName();
    }
  }
  return {lib, box, file, type};
}

Maybe<AccessInfo> Context::getReqInfoIfDifferentModule(IR::QatModule* otherMod) const {
  if ((getMod()->getID() == otherMod->getID()) || getMod()->isParentModuleOf(otherMod)) {
    return None;
  } else {
    return getAccessInfo();
  }
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
  Json               result;
  unsigned long long qatCompileTime = 0;
  unsigned long long clangTime      = 0;
  if (qatStartTime) {
    qatCompileTime = std::chrono::duration_cast<std::chrono::microseconds>(
                         qatEndTime.value_or(std::chrono::high_resolution_clock::now()) - qatStartTime.value())
                         .count();
  }
  if (clangLinkStartTime) {
    clangTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    clangLinkEndTime.value_or(std::chrono::high_resolution_clock::now()) - clangLinkStartTime.value())
                    .count();
  }
  SHOW("Setting binary sizes in result")
  Vec<JsonValue> binarySizesJson;
  for (auto binSiz : binarySizes) {
    binarySizesJson.push_back(binSiz);
  }
  SHOW("Creating JSON object for result")
  result._("status", status)
      ._("problems", problems)
      ._("lexerTime", (unsigned long long)lexer::Lexer::timeInMicroSeconds)
      ._("parserTime", (unsigned long long)parser::Parser::timeInMicroSeconds)
      ._("compilationTime", qatCompileTime)
      ._("linkingTime", clangTime)
      ._("binarySizes", binarySizesJson)
      ._("hasMain", hasMain);
  std::fstream output;
  output.open((cli::Config::get()->getOutputPath() / "QatCompilationResult.json").string().c_str(), std::ios_base::out);
  if (output.is_open()) {
    output << result;
    output.close();
  }
}

bool Context::moduleAlreadyHasErrors(IR::QatModule* cand) {
  for (auto* module : modulesWithErrors) {
    if (module->getID() == cand->getID()) {
      return true;
    }
  }
  return false;
}

clang::LangAS Context::getProgramAddressSpaceAsLangAS() const {
  if (dataLayout) {
    return clang::getLangASFromTargetAS(dataLayout->getProgramAddressSpace());
  } else {
    return clang::LangAS::Default;
  }
}

bool Context::hasActiveGeneric() const { return !allActiveGenerics.empty(); }

GenericEntityMarker& Context::getActiveGeneric() const { return allActiveGenerics.back(); }

void Context::addActiveGeneric(GenericEntityMarker marker, bool main) {
  SHOW("ADDED ACTIVE GENERIC")
  if (main) {
    lastMainActiveGeneric.push_back(allActiveGenerics.size());
  }
  allActiveGenerics.push_back(marker);
}

void Context::removeActiveGeneric() {
  SHOW("REMOVED ACTIVE GENERICS")
  if ((!lastMainActiveGeneric.empty()) && (allActiveGenerics.size() - 1 == lastMainActiveGeneric.back())) {
    lastMainActiveGeneric.pop_back();
  }
  allActiveGenerics.pop_back();
}

void Context::addError(const String& message, Maybe<FileRange> fileRange) {
  auto* cfg = cli::Config::get();
  if (hasActiveGeneric()) {
    codeProblems.push_back(CodeProblem(true,
                                       "Errors generated while creating generic variant: " + getActiveGeneric().name,
                                       getActiveGeneric().fileRange));
    std::cerr << "\n"
              << Colored(colors::highIntensityBackground::red) << " ERROR " << Colored(colors::cyan) << " --> "
              << Colored(colors::reset) << getActiveGeneric().fileRange.file.string() << ":"
              << getActiveGeneric().fileRange.start.line << ":" << getActiveGeneric().fileRange.start.character << "\n"
              << "Errors while creating generic variant: " << highlightError(getActiveGeneric().name) << "\n"
              << "\n";
  }
  codeProblems.push_back(CodeProblem(
      true, (hasActiveGeneric() ? ("Creating " + getActiveGeneric().name + " => ") : "") + message, fileRange));
  std::cerr << "\n" << Colored(colors::highIntensityBackground::red) << " ERROR ";
  if (fileRange) {
    std::cerr << Colored(colors::cyan) << " --> " << Colored(colors::reset) << fileRange.value().file.string() << ":"
              << fileRange.value().start.line << ":" << fileRange.value().start.character;
  }
  std::cerr << Colored(colors::bold::white) << "\n"
            << (hasActiveGeneric() ? ("Creating " + getActiveGeneric().name + " => ") : "") << message
            << Colored(colors::reset) << "\n";
  if (fileRange) {
    printRelevantFileContent(fileRange.value(), true);
  }
  std::cerr << "\n";
  if (mod && !moduleAlreadyHasErrors(mod)) {
    if (hasActiveGeneric()) {
      removeActiveGeneric();
    }
    modulesWithErrors.push_back(mod);
    for (const auto& modNRange : mod->getBroughtMentions()) {
      mod = modNRange.first;
      addError("Error occured in this file", modNRange.second);
    }
  }
}

void Context::printRelevantFileContent(FileRange const& fileRange, bool isError) const {
  auto cfg = cli::Config::get();
  if (!fs::is_regular_file(fileRange.file)) {
    return;
  }
  auto lines = getContentForDiagnostics(fileRange);
  for (auto& lineInfo : lines) {
    std::cerr << "▌ " << std::get<0>(lineInfo) << "\n";
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
      auto indicatorCount = std::get<2>(lineInfo) - std::get<1>(lineInfo);
      if (indicatorCount > 0) {
        indicatorCount--;
      }
      String indicator(indicatorCount, '^');
      std::cerr << "▌ " << spacing << (isError ? Colored(colors::bold::red) : Colored(colors::bold::purple))
                << indicator << Colored(colors::reset) << "\n";
    }
  }
}

void Context::Error(const String& message, Maybe<FileRange> fileRange) {
  addError(message, fileRange);
  writeJsonResult(false);
  sitter->destroy();
  delete cli::Config::get();
  MemoryTracker::report();
  ast::Node::clearAll();
  exit(0);
}

Vec<std::tuple<String, u64, u64>> Context::getContentForDiagnostics(FileRange const& _range) const {
  Vec<std::tuple<String, u64, u64>> result;
  std::ifstream                     file(_range.file);
  String                            line;
  u64                               lineCount = 0;
  while (std::getline(file, line)) {
    if ((_range.start.line > 0u) && (lineCount == (_range.start.line - 2))) {
      // Line before the first relevant line in file range
      result.push_back({line, 0u, 0u});
    } else if ((lineCount >= _range.start.line - 1) && (lineCount < _range.end.line)) {
      // Relevant line
      if (_range.start.line == _range.end.line) {
        result.push_back({line, _range.start.character - 1, _range.end.character});
      } else if (lineCount == _range.start.line) {
        result.push_back({line, _range.start.character, line.size() - 1});
      } else if (lineCount == _range.end.line) {
        result.push_back({line, 0u, _range.end.character});
      } else {
        result.push_back({line, 0u, line.size() - 1});
      }
    } else if ((_range.start.line <= UINT64_MAX) && (lineCount == (_range.end.line))) {
      // Line after the last relevant line in file range
      result.push_back({line, 0u, 0u});
      break;
    }
    lineCount++;
  }
  return result;
}

String Context::joinActiveGenericNames(bool highlight) const {
  String result;
  for (usize i = 0; i < allActiveGenerics.size(); i++) {
    result.append(highlight ? highlightError(allActiveGenerics.at(i).name) : allActiveGenerics.at(i).name);
    if (i < allActiveGenerics.size() - 1) {
      result.append(" and ");
    }
  }
  return result;
}

void Context::Warning(const String& message, const FileRange& fileRange) const {
  if (hasActiveGeneric()) {
    getActiveGeneric().warningCount++;
  }
  codeProblems.push_back(CodeProblem(
      false, (hasActiveGeneric() ? ("Creating " + joinActiveGenericNames(false) + " => ") : "") + message, fileRange));
  auto* cfg = cli::Config::get();
  std::cout << "\n"
            << Colored(colors::highIntensityBackground::purple) << " WARNING " << Colored(colors::cyan) << " --> "
            << Colored(colors::reset) << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << Colored(colors::bold::white) << "\n"
            << (hasActiveGeneric() ? ("Creating " + joinActiveGenericNames(true) + " => ") : "") << message
            << Colored(colors::reset) << "\n";
  printRelevantFileContent(fileRange, false);
}

} // namespace qat::IR
