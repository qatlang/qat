#ifndef QAT_IR_CONTEXT_HPP
#define QAT_IR_CONTEXT_HPP

#include "../utils/file_range.hpp"
#include "./qat_module.hpp"

#include <string>
#include <vector>

namespace qat::IR {

class Context {
public:
  Context();

  // The IR module
  QatModule *mod;

  // Get the active IR module
  QatModule *getActive();

  // All the boxes exposed in the current scope. This will automatically
  // be populated and de-populated when the expose scope starts and ends
  std::vector<std::string> exposed;

  void throw_error(const std::string message, const utils::FileRange fileRange);
};

} // namespace qat::IR

#endif