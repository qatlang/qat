#include "./tuple_value.hpp"

namespace qat {
namespace AST {

TupleValue::TupleValue(std::vector<Expression *> _members,
                       utils::FilePlacement _filePlacement)
    : members(_members), Expression(_filePlacement) {}

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
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat