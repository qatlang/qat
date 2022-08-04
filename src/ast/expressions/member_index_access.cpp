#include "./member_index_access.hpp"

namespace qat::ast {

MemberIndexAccess::MemberIndexAccess(Expression *_instance, Expression *_index,
                                     utils::FileRange _fileRange)
    : instance(_instance), index(_index), Expression(std::move(_fileRange)) {}

IR::Value *MemberIndexAccess::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json MemberIndexAccess::toJson() const {
  return nuo::Json()
      ._("nodeType", "memberIndexAccess")
      ._("instance", instance->toJson())
      ._("index", index->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast