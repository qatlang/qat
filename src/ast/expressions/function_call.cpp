#include "./function_call.hpp"

namespace qat::ast {

IR::Value *FunctionCall::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json FunctionCall::toJson() const {
  Vec<nuo::JsonValue> args;
  for (auto arg : arguments) {
    args.emplace_back(arg->toJson());
  }
  return nuo::Json()
      ._("nodeType", "functionCall")
      ._("function", name)
      ._("arguments", args)
      ._("fileRange", fileRange);
}

} // namespace qat::ast