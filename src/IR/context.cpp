#include "./context.hpp"
#include "../ast/node.hpp"
#include "../cli/color.hpp"
#include "../cli/config.hpp"
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

CodeProblem::CodeProblem(bool _isError, String _message, utils::FileRange _range)
    : isError(_isError), message(std::move(_message)), range(std::move(_range)) {}

CodeProblem::operator Json() const { return Json()._("isError", isError)._("message", message)._("range", range); }

Context::Context() : builder(llctx), hasMain(false) {}

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
        return utils::VisibilityInfo::folder(fs::path(getMod()->getFilePath()).parent_path());
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
          return utils::VisibilityInfo::folder(fs::path(getMod()->getParentFile()->getFilePath()).parent_path());
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
                  qatEndTime.value_or(std::chrono::steady_clock::now()) - qatStartTime.value())
                  .count();
  }
  if (clangLinkStartTime) {
    clangTime = std::chrono::duration_cast<std::chrono::microseconds>(
                    clangLinkEndTime.value_or(std::chrono::steady_clock::now()) - clangLinkStartTime.value())
                    .count();
  }
  result._("status", status)
      ._("problems", problems)
      ._("qatTime", qatTime)
      ._("clangTime", clangTime)
      ._("hasMain", hasMain);
  std::fstream output;
  output.open((cli::Config::get()->getOutputPath() / "qat_result.json").string().c_str(), std::ios_base::out);
  if (output.is_open()) {
    output << result;
    output.close();
  }
}

void Context::Error(const String& message, const utils::FileRange& fileRange) const {
  auto* cfg = cli::Config::get();
  if (activeTemplate) {
    codeProblems.push_back(CodeProblem(true, "Errors generated while creating generic variant: " + activeTemplate->name,
                                       activeTemplate->fileRange));
    std::cout << Colored(colors::highIntensityBackground::red) << "  error  " << Colored(colors::white) << "▌"
              << Colored(colors::reset) << " " << Colored(colors::bold::red)
              << "Errors generated while creating generic variant: " << highlightError(activeTemplate->name)
              << Colored(colors::reset) << " | " << Colored(colors::underline::green)
              << activeTemplate->fileRange.file.string() << ":" << activeTemplate->fileRange.start.line << ":"
              << activeTemplate->fileRange.start.character << Colored(colors::reset) << " >> "
              << Colored(colors::underline::green) << activeTemplate->fileRange.file.string() << ":"
              << activeTemplate->fileRange.end.line << ":" << activeTemplate->fileRange.end.character
              << Colored(colors::reset) << "\n";
  }
  codeProblems.push_back(
      CodeProblem(true, (activeTemplate ? ("Creating " + activeTemplate->name + " => ") : "") + message, fileRange));
  std::cout << Colored(colors::highIntensityBackground::red) << "  error  " << Colored(colors::white) << "▌"
            << Colored(colors::reset) << " " << Colored(colors::bold::red)
            << (activeTemplate ? ("Creating " + activeTemplate->name + " => ") : "") << message
            << Colored(colors::reset) << " | " << Colored(colors::underline::green) << fileRange.file.string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character << Colored(colors::reset) << " >> "
            << Colored(colors::underline::green) << fileRange.file.string() << ":" << fileRange.end.line << ":"
            << fileRange.end.character << Colored(colors::reset) << "\n";
  ast::Node::clearAll();
  Value::clearAll();
  writeJsonResult(false);
  exit(0);
}

void Context::Warning(const String& message, const utils::FileRange& fileRange) const {
  if (activeTemplate) {
    activeTemplate->warningCount++;
  }
  codeProblems.push_back(
      CodeProblem(false, (activeTemplate ? ("Creating " + activeTemplate->name + " => ") : "") + message, fileRange));
  auto* cfg = cli::Config::get();
  std::cout << Colored(colors::highIntensityBackground::purple) << " warning " << Colored(colors::blue) << "▌"
            << Colored(colors::reset) << " " << Colored(colors::bold::purple)
            << (activeTemplate ? ("Creating " + activeTemplate->name + " => ") : "") << message
            << Colored(colors::reset) << " | " << Colored(colors::underline::green) << fileRange.file.string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character << Colored(colors::reset) << " >> "
            << Colored(colors::underline::green) << fileRange.file.string() << ":" << fileRange.end.line << ":"
            << fileRange.end.character << Colored(colors::reset) << "\n";
}

} // namespace qat::IR