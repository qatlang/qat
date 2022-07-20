#include "./member_function_call.hpp"

namespace qat::ast {

IR::Value *MemberFunctionCall::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json MemberFunctionCall::toJson() const {
  std::vector<nuo::JsonValue> args;
  for (auto arg : arguments) {
    args.push_back(arg->toJson());
  }
  return nuo::Json()
      ._("nodeType", "memberFunctionCall")
      ._("instance", instance->toJson())
      ._("function", memberName)
      ._("arguments", args)
      ._("isVariation", variation)
      ._("fileRange", fileRange);
}

} // namespace qat::ast