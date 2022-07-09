#include "./say_sentence.hpp"

namespace qat {
namespace AST {

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

backend::JSON Say::toJSON() const {
  std::vector<backend::JSON> exps;
  for (auto exp : expressions) {
    exps.push_back(exp->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "saySentence")
      ._("values", exps)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat