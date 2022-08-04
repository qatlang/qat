#include "./context.hpp"
#include "../cli/color.hpp"
#include <iostream>

namespace qat::IR {

Context::Context() : mod(nullptr) {}

QatModule *Context::getActive() { return mod->getActive(); }

void Context::Error(const String &message, const utils::FileRange &fileRange) {
  std::cout << colors::red << "[ COMPILER ERROR ] " << colors::bold::green
            << fileRange.file.string() << ":" << fileRange.start.line << ":"
            << fileRange.start.character << "-" << fileRange.end.line << ":"
            << fileRange.end.character << colors::reset << "\n"
            << "   " << message << "\n";
  exit(0);
}

} // namespace qat::IR