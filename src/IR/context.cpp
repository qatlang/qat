#include "./context.hpp"
#include "../ast/node.hpp"
#include "../cli/color.hpp"
#include "./value.hpp"
#include "member_function.hpp"
#include <iostream>

namespace qat::IR {

LoopInfo::LoopInfo(String _name, IR::Block* _mainB, IR::Block* _condB, IR::Block* _restB, IR::LocalValue* _index,
                   LoopType _type)
    : name(std::move(_name)), mainBlock(_mainB), condBlock(_condB), restBlock(_restB), index(_index), type(_type) {}

bool LoopInfo::isTimes() const { return type == LoopType::nTimes; }

Breakable::Breakable(Maybe<String> _tag, IR::Block* _restBlock, IR::Block* _trueBlock)
    : tag(std::move(_tag)), restBlock(_restBlock), trueBlock(_trueBlock) {}

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

String Context::highlightError(const String& message, const char* color) { return color + message + colors::bold::red; }

String Context::highlightWarning(const String& message, const char* color) {
  return color + message + colors::bold::purple;
}

void Context::Error(const String& message, const utils::FileRange& fileRange) const {
  if (activeTemplate) {
    std::cout << colors::highIntensityBackground::red << "  error  " << colors::white << "▌" << colors::reset << " "
              << colors::bold::red
              << "Errors generated while creating template variant: " << highlightError(activeTemplate->name)
              << colors::reset << " | " << colors::underline::green << activeTemplate->fileRange.file.string() << ":"
              << activeTemplate->fileRange.start.line << ":" << activeTemplate->fileRange.start.character
              << colors::reset << " >> " << colors::underline::green << activeTemplate->fileRange.file.string() << ":"
              << activeTemplate->fileRange.end.line << ":" << activeTemplate->fileRange.end.character << colors::reset
              << "\n";
  }
  std::cout << colors::highIntensityBackground::red << "  error  " << colors::white << "▌" << colors::reset << " "
            << colors::bold::red << (activeTemplate ? ("Generating " + activeTemplate->name + " => ") : "") << message
            << colors::reset << " | " << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":" << fileRange.end.line << ":"
            << fileRange.end.character << colors::reset << "\n";
  ast::Node::clearAll();
  Value::clearAll();
  exit(0);
}

void Context::Warning(const String& message, const utils::FileRange& fileRange) const {
  if (activeTemplate) {
    activeTemplate->warningCount++;
  }
  std::cout << colors::highIntensityBackground::purple << " warning " << colors::blue << "▌" << colors::reset << " "
            << colors::bold::purple << (activeTemplate ? ("Generating " + activeTemplate->name + " => ") : "")
            << message << colors::reset << " | " << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":" << fileRange.end.line << ":"
            << fileRange.end.character << colors::reset << "\n";
}

} // namespace qat::IR