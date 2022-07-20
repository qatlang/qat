#include "./member_variable.hpp"

namespace qat::ast {

IR::Value *MemberVariable::emit(IR::Context *ctx) {
  // FIXME - Implement this
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