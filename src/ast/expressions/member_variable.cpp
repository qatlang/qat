#include "./member_variable.hpp"

namespace qat::ast {

MemberVariable::MemberVariable(Expression *_instance, bool _isPointerAccess,
                               String _memberName, utils::FileRange _fileRange)
    : instance(_instance), isPointerAccess(_isPointerAccess),
      memberName(std::move(_memberName)), Expression(std::move(_fileRange)) {}

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