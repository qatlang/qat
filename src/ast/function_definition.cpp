#include "./function_definition.hpp"
#include "../show.hpp"

namespace qat::ast {

FunctionDefinition::FunctionDefinition(FunctionPrototype *_prototype,
                                       std::vector<Sentence *> _sentences,
                                       utils::FileRange _fileRange)
    : prototype(_prototype), sentences(_sentences), Node(_fileRange) {}

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

nuo::Json FunctionDefinition::toJson() const {
  std::vector<nuo::JsonValue> sntcs;
  for (auto sentence : sentences) {
    auto sjson = sentence->toJson();
    sntcs.push_back(sjson);
  }
  return nuo::Json()
      ._("nodeType", "functionDefinition")
      ._("prototype", prototype->toJson())
      ._("body", sntcs)
      ._("fileRange", fileRange);
}

} // namespace qat::ast