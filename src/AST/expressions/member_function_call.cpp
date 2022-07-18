#include "./member_function_call.hpp"

namespace qat::AST {

IR::Value *MemberFunctionCall::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void MemberFunctionCall::emitCPP(backend::cpp::File &file,
                                 bool isHeader) const {
  if (!isHeader) {
    file += "(";
    instance->emitCPP(file, isHeader);
    file += "." + memberName + "(";
    for (std::size_t i = 0; i < arguments.size(); i++) {
      arguments.at(i)->emitCPP(file, isHeader);
      if (i != (arguments.size() - 1)) {
        file += ", ";
      }
    }
    file += ")) ";
  }
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
      ._("filePlacement", fileRange);
}

} // namespace qat::AST