#include "./member_index_access.hpp"

namespace qat {
namespace AST {

IR::Value *MemberIndexAccess::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void MemberIndexAccess::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += " (";
    instance->emitCPP(file, isHeader);
    file += "[";
    index->emitCPP(file, isHeader);
    file += "]) ";
  }
}

backend::JSON MemberIndexAccess::toJSON() const {
  return backend::JSON()
      ._("nodeType", "memberIndexAccess")
      ._("instance", instance->toJSON())
      ._("index", index->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat