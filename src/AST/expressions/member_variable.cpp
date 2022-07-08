#include "./member_variable.hpp"

namespace qat {
namespace AST {

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

backend::JSON MemberVariable::toJSON() const {
  return backend::JSON()
      ._("nodeType", "memberVariable")
      ._("instance", instance->toJSON())
      ._("isPointerAccess", isPointerAccess)
      ._("member", memberName)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat