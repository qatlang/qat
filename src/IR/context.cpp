#include "./context.hpp"
#include "../CLI/color.hpp"
#include <iostream>

namespace qat::IR {

Context::Context() : mod(nullptr) {}

QatModule *Context::getActive() { return mod->getActive(); }

void Context::throw_error(const std::string message,
                          const utils::FileRange placement) {
  std::cout << colors::red << "[ COMPILER ERROR ] " << colors::bold::green
            << placement.file.string() << ":" << placement.start.line << ":"
            << placement.start.character << " - " << placement.end.line << ":"
            << placement.end.character << colors::reset << "\n"
            << "   " << message << "\n";
  exit(0);
}

} // namespace qat::IR