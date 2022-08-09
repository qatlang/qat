#include "./context.hpp"
#include "../cli/color.hpp"
#include <iostream>

namespace qat::IR {

Context::Context() : mod(nullptr), builder(llctx), hasMain(false) {}

QatModule *Context::getMod() const { return mod->getActive(); }

void Context::Error(const String &message, const utils::FileRange &fileRange) {
  std::cout << colors::highIntensityBackground::red << " error "
            << colors::reset << " " << colors::bold::red << message
            << colors::reset << " | " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.end.line << ":" << fileRange.end.character
            << colors::reset << "\n";
  exit(0);
}

void Context::Warning(const String           &message,
                      const utils::FileRange &fileRange) {
  std::cout << colors::highIntensityBackground::yellow << " warning "
            << colors::reset << " " << colors::bold::yellow << message
            << colors::reset << " | " << colors::underline::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << colors::reset << " >> "
            << colors::underline::green << fileRange.file.string() << ":"
            << fileRange.end.line << ":" << fileRange.end.character
            << colors::reset << "\n";
}

} // namespace qat::IR