#include "./tuple_value.hpp"

namespace qat::ast {

TupleValue::TupleValue(Vec<Expression *> _members, utils::FileRange _fileRange)
    : members(_members), Expression(_fileRange) {}

IR::Value *TupleValue::emit(IR::Context *ctx) {
  // TODO - Implement this
}

Json TupleValue::toJson() const {
  Vec<JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return Json()
      ._("nodeType", "tupleValue")
      ._("members", mems)
      ._("fileRange", fileRange);
}

} // namespace qat::ast