#include "./member_function_call.hpp"
namespace qat {
namespace AST {

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

backend::JSON MemberFunctionCall::toJSON() const {
  std::vector<backend::JSON> args;
  for (auto arg : arguments) {
    args.push_back(arg->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "memberFunctionCall")
      ._("instance", instance->toJSON())
      ._("function", memberName)
      ._("arguments", args)
      ._("isVariation", variation)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat