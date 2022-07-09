#include "./function_definition.hpp"
#include "../show.hpp"

namespace qat {
namespace AST {

FunctionDefinition::FunctionDefinition(FunctionPrototype *_prototype,
                                       std::vector<Sentence *> _sentences,
                                       utils::FilePlacement _filePlacement)
    : prototype(_prototype), sentences(_sentences), Node(_filePlacement) {}

IR::Value *FunctionDefinition::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void FunctionDefinition::emitCPP(backend::cpp::File &file,
                                 bool isHeader) const {
  prototype->emitCPP(file, isHeader);
  if (!isHeader) {
    file += " {\n";
    for (auto snt : sentences) {
      file += "  ";
      snt->emitCPP(file, isHeader);
    }
    file += "\n}\n";
  }
}

backend::JSON FunctionDefinition::toJSON() const {
  std::vector<backend::JSON> sntcs;
  for (auto sentence : sentences) {
    sntcs.push_back(sentence->toJSON());
  }
  return backend::JSON()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJSON())
      ._("body", sntcs)
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat