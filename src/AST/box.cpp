#include "./box.hpp"
#include <nuo/json.hpp>

namespace qat::AST {

IR::Value *Box::emit(IR::Context *ctx) {
  // FIXME - Perform name checks
  // TODO - Implement this
}

void Box::emitCPP(backend::cpp::File &file, bool isHeader) const {
  file += ("namespace " + name + " ");
  file.addEnclosedComment("box");
  file += " {\n";
  for (auto mem : members) {
    mem->emitCPP(file, isHeader);
  }
  file += "} // namespace " + name + " (box)\n";
}

nuo::Json Box::toJson() const {
  std::vector<nuo::JsonValue> mems;
  for (auto mem : members) {
    mems.push_back(mem->toJson());
  }
  return nuo::Json()
      ._("nodeType", "box")
      ._("members", mems)
      ._("filePlacement", fileRange);
}

} // namespace qat::AST