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
#include <chrono>
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

CodeProblem::CodeProblem(bool _isError, String _message, FileRange _range)
    : isError(_isError), message(std::move(_message)), range(std::move(_range)) {}

CodeProblem::operator Json() const { return Json()._("isError", isError)._("message", message)._("range", range); }

Context::Context() : builder(llctx), hasMain(false) { llctx.setOpaquePointers(true); }

Context::~Context() {
  for (auto* lInfo : loopsInfo) {
    delete lInfo;
  }
  for (auto* brk : breakables) {
    delete brk;
  }
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
  auto reqInfo = getReqInfo();
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
    if (genericID.has_value() && mod->getGenericCoreType(name.value, getReqInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic core type named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtGenericCoreType(name.value, None)) {
    if (genericID.has_value() && mod->getGenericCoreType(name.value, getReqInfo())->getID() == genericID.value()) {
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
    if (genericID.has_value() && mod->getGenericFunction(name.value, getReqInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) +
              " exists in this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasBroughtGenericFunction(name.value, None)) {
    if (genericID.has_value() && mod->getGenericFunction(name.value, getReqInfo())->getID() == genericID.value()) {
      return;
    }
    Error("A generic function named " + highlightError(name.value) +
              " is brought into this module. Please change name of this " + entityType +
              " or check the codebase for inconsistencies",
          name.range);
  } else if (mod->hasAccessibleGenericFunctionInImports(name.value, reqInfo).first) {
    if (genericID.has_value() && mod->getGenericFunction(name.value, getReqInfo())->getID() == genericID.value()) {
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

utils::VisibilityInfo Context::getVisibInfo(Maybe<utils::VisibilityKind> kind) const {
  if (kind.has_value() && (kind.value() != utils::VisibilityKind::parent)) {
    SHOW("Visibility kind has value")
    switch (kind.value()) {
      case utils::VisibilityKind::box: {
        return utils::VisibilityInfo::box(getMod()->getClosestParentBox()->getFullName());
      }
      case utils::VisibilityKind::lib: {
        return utils::VisibilityInfo::lib(getMod()->getClosestParentLib()->getFullName());
      }
      case utils::VisibilityKind::file: {
        return utils::VisibilityInfo::file(getMod()->getFilePath());
      }
      case utils::VisibilityKind::folder: {
        return utils::VisibilityInfo::folder(fs::path(getMod()->getFilePath()).parent_path().string());
      }
      case utils::VisibilityKind::type: {
        if (activeType) {
          return utils::VisibilityInfo::type(activeType->getFullName());
        } else {
          if (fn && fn->isMemberFunction()) {
            return utils::VisibilityInfo::type(((IR::MemberFunction*)fn)->getParentType()->getFullName());
          } else {
            return utils::VisibilityInfo::type("");
          }
        }
        // TODO - Handle in case it is null
      }
      case utils::VisibilityKind::pub: {
        return utils::VisibilityInfo::pub();
      }
      default:
        break;
    }
  } else {
    SHOW("No visibility kind")
    if (activeType) {
      SHOW("Found active type")
      return utils::VisibilityInfo::type(activeType->getFullName());
    } else {
      SHOW("No active type")
      switch (getMod()->getModuleType()) {
        case ModuleType::box: {
          return utils::VisibilityInfo::box(getMod()->getFullName());
        }
        case ModuleType::file: {
          return utils::VisibilityInfo::file(getMod()->getFilePath());
        }
        case ModuleType::lib: {
          return utils::VisibilityInfo::lib(getMod()->getFullName());
        }
        case ModuleType::folder: {
          return utils::VisibilityInfo::folder(
              fs::path(getMod()->getParentFile()->getFilePath()).parent_path().string());
        }
      }
    }
  }
  SHOW("No visibility info found")
} // NOLINT(clang-diagnostic-return-type)

utils::RequesterInfo Context::getReqInfo() const {
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

Maybe<utils::RequesterInfo> Context::getReqInfoIfDifferentModule(IR::QatModule* otherMod) const {
  if ((getMod()->getID() == otherMod->getID()) || getMod()->isParentModuleOf(otherMod)) {
    return None;
  } else {
    return getReqInfo();
  }
}

String Context::highlightError(const String& message, const char* color) {
  auto* cfg = cli::Config::get();
  return ColoredOr(color, "`") + message + ColoredOr(colors::bold::red, "`");
}

String Context::highlightWarning(const String& message, const char* color) {
  auto* cfg = cli::Config::get();
  return ColoredOr(color, "`") + message + ColoredOr(colors::bold::purple, "`");
}

void Context::writeJsonResult(bool status) const {
  Vec<JsonValue> problems;
  for (const auto& prob : codeProblems) {
    problems.push_back((Json)prob);
  }
  Json               result;
  unsigned long long qatTime   = 0;
  unsigned long long clangTime = 0;
  if (qatStartTime) {
    qatTime = std::chrono::duration_cast<std::chrono::microseconds>(
                  qatEndTime.value_or(std::chrono::high_resolution_clock::now()) - qatStartTime.value())
                  .count();
  }
  if (clangLinkStartTime) {
    clangTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    clangLinkEndTime.value_or(std::chrono::high_resolution_clock::now()) - clangLinkStartTime.value())
                    .count();
  }
  result._("status", status)
      ._("problems", problems)
      ._("lexerTime", (unsigned long long)lexer::Lexer::timeInMicroSeconds)
      ._("parserTime", (unsigned long long)parser::Parser::timeInMicroSeconds)
      ._("qatTime", qatTime)
      ._("clangTime", clangTime)
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

void Context::addError(const String& message, const FileRange& fileRange) {
  auto* cfg = cli::Config::get();
  if (activeGeneric) {
    codeProblems.push_back(CodeProblem(true, "Errors generated while creating generic variant: " + activeGeneric->name,
                                       activeGeneric->fileRange));
    std::cout << Colored(colors::highIntensityBackground::red) << "  error  " << Colored(colors::white) << "▌"
              << Colored(colors::reset) << " " << Colored(colors::bold::red)
              << "Errors generated while creating generic variant: " << highlightError(activeGeneric->name)
              << Colored(colors::reset) << " | " << Colored(colors::underline::green)
              << activeGeneric->fileRange.file.string() << ":" << activeGeneric->fileRange.start.line << ":"
              << activeGeneric->fileRange.start.character << Colored(colors::reset) << " >> "
              << Colored(colors::underline::green) << activeGeneric->fileRange.file.string() << ":"
              << activeGeneric->fileRange.end.line << ":" << activeGeneric->fileRange.end.character
              << Colored(colors::reset) << "\n";
  }
  codeProblems.push_back(
      CodeProblem(true, (activeGeneric ? ("Creating " + activeGeneric->name + " => ") : "") + message, fileRange));
  std::cout << Colored(colors::highIntensityBackground::red) << "  error  " << Colored(colors::white) << "▌"
            << Colored(colors::reset) << " " << Colored(colors::bold::red)
            << (activeGeneric ? ("Creating " + activeGeneric->name + " => ") : "") << message << Colored(colors::reset)
            << " | " << Colored(colors::underline::green) << fileRange.file.string() << ":" << fileRange.start.line
            << ":" << fileRange.start.character << Colored(colors::reset) << " >> " << Colored(colors::underline::green)
            << fileRange.file.string() << ":" << fileRange.end.line << ":" << fileRange.end.character
            << Colored(colors::reset) << "\n";
  if (!moduleAlreadyHasErrors(mod)) {
    activeGeneric = None;
    modulesWithErrors.push_back(mod);
    for (const auto& modNRange : mod->getBroughtMentions()) {
      mod = modNRange.first;
      addError("Error occured in this file", modNRange.second);
    }
  }
}

void Context::Error(const String& message, const FileRange& fileRange) {
  addError(message, fileRange);
  writeJsonResult(false);
  sitter->destroy();
  delete cli::Config::get();
  MemoryTracker::report();
  exit(0);
}

void Context::Warning(const String& message, const FileRange& fileRange) const {
  if (activeGeneric) {
    activeGeneric->warningCount++;
  }
  codeProblems.push_back(
      CodeProblem(false, (activeGeneric ? ("Creating " + activeGeneric->name + " => ") : "") + message, fileRange));
  auto* cfg = cli::Config::get();
  std::cout << Colored(colors::highIntensityBackground::purple) << " warning " << Colored(colors::blue) << "▌"
            << Colored(colors::reset) << " " << Colored(colors::bold::purple)
            << (activeGeneric ? ("Creating " + activeGeneric->name + " => ") : "") << message << Colored(colors::reset)
            << " | " << Colored(colors::underline::green) << fileRange.file.string() << ":" << fileRange.start.line
            << ":" << fileRange.start.character << Colored(colors::reset) << " >> " << Colored(colors::underline::green)
            << fileRange.file.string() << ":" << fileRange.end.line << ":" << fileRange.end.character
            << Colored(colors::reset) << "\n";
}

} // namespace qat::IR