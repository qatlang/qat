#include "./function_call.hpp"

namespace qat {
namespace AST {

IR::Value *qat::AST::FunctionCall::emit(qat::IR::Context *ctx) {
  // TODO - Implement this
}

void FunctionCall::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += (" " + name + "(");
    for (std::size_t i = 0; i < arguments.size(); i++) {
      arguments.at(i)->emitCPP(file, isHeader);
      if (i != (arguments.size() - 1)) {
        file += ", ";
      }
    }
    file += ")";
  }
}

backend::JSON FunctionCall::toJSON() const {
  std::vector<backend::JSON> args;
  for (auto arg : arguments) {
    args.push_back(arg->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "functionCall")
      ._("function", name)
      ._("arguments", args)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat