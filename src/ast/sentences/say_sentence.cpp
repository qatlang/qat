#include "./say_sentence.hpp"

namespace qat::ast {

IR::Value *Say::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void Say::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file.addInclude("<iostream>");
    file += "std::cout << ";
    for (std::size_t i = 0; i < expressions.size(); i++) {
      expressions.at(i)->emitCPP(file, isHeader);
      if (i != (expressions.size() - 1)) {
        file += " << ";
      }
    }
    file += ";\n";
  }
}

nuo::Json Say::toJson() const {
  std::vector<nuo::JsonValue> exps;
  for (auto exp : expressions) {
    exps.push_back(exp->toJson());
  }
  return nuo::Json()
      ._("nodeType", "saySentence")
      ._("values", exps)
      ._("fileRange", fileRange);
}

} // namespace qat::ast