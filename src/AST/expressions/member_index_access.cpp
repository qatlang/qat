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

nuo::Json MemberIndexAccess::toJson() const {
  return nuo::Json()
      ._("nodeType", "memberIndexAccess")
      ._("instance", instance->toJson())
      ._("index", index->toJson())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat