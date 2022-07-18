#include "./member_variable.hpp"

namespace qat::ast {

IR::Value *MemberVariable::emit(IR::Context *ctx) {
  // FIXME - Implement this
}

void MemberVariable::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "(";
    instance->emitCPP(file, isHeader);
    file += (isPointerAccess ? "->" : ".") + memberName + ")";
  }
}

nuo::Json MemberVariable::toJson() const {
  return nuo::Json()
      ._("nodeType", "memberVariable")
      ._("instance", instance->toJson())
      ._("isPointerAccess", isPointerAccess)
      ._("member", memberName)
      ._("fileRange", fileRange);
}

} // namespace qat::ast