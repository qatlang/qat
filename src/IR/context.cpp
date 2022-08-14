#include "./context.hpp"
#include "../ast/node.hpp"
#include "../cli/color.hpp"
#include "./value.hpp"
#include "member_function.hpp"
#include <iostream>

namespace qat::IR {

Context::Context() : builder(llctx), mod(nullptr), hasMain(false) {}

QatModule *Context::getMod() const { return mod->getActive(); }

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
      type = ((MemberFunction *)fn)->getParentType()->getFullName();
    }
  }
  return {lib, box, file, type};
}

String Context::highlightError(const String &message, const char *color) {
  return color + message + colors::bold::red;
}

String Context::highlightWarning(const String &message, const char *color) {
  return color + message + colors::bold::purple;
}

void Context::Error(const String &message, const utils::FileRange &fileRange) {
  std::cout << colors::highIntensityBackground::red << "  error  "
            << colors::white << "▌" << colors::reset << " " << colors::bold::red
            << message << colors::reset << " | " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.end.line << ":" << fileRange.end.character
            << colors::reset << "\n";
  ast::Node::clearAll();
  Value::clearAll();
  exit(0);
}

void Context::Warning(const String           &message,
                      const utils::FileRange &fileRange) {
  std::cout << colors::highIntensityBackground::purple << " warning "
            << colors::blue << "▌" << colors::reset << " "
            << colors::bold::purple << message << colors::reset << " | "
            << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.start.line << ":" << fileRange.start.character
            << colors::reset << " >> " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.end.line << ":"
            << fileRange.end.character << colors::reset << "\n";
}

} // namespace qat::IR