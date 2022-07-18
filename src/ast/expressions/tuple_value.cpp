#include "./tuple_value.hpp"

namespace qat::ast {

TupleValue::TupleValue(std::vector<Expression *> _members,
                       utils::FileRange _fileRange)
    : members(_members), Expression(_fileRange) {}

IR::Value *TupleValue::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void TupleValue::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file.addInclude("<tuple>");
    file += "std::tuple(";
    for (std::size_t i = 0; i < members.size(); i++) {
      members.at(i)->emitCPP(file, isHeader);
      if (i != (members.size() - 1)) {
        file += ", ";
      }
    }
    file += ") ";
  }
}

nuo::Json TupleValue::toJson() const {
  std::vector<nuo::JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()
      ._("nodeType", "tupleValue")
      ._("members", mems)
      ._("fileRange", fileRange);
}

} // namespace qat::ast