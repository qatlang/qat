#include "./size_of_type.hpp"

namespace qat::AST {

SizeOfType::SizeOfType(QatType *_type, utils::FileRange _filePlacement)
    : type(_type), Expression(_filePlacement) {}

IR::Value *SizeOfType::emit(IR::Context *ctx) {
  // TODO - Implement this
}

void SizeOfType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += "sizeof(";
    type->emitCPP(file, isHeader);
    file += ")";
  }
}

nuo::Json SizeOfType::toJson() const {
  return nuo::Json()
      ._("nodeType", "sizeOfType")
      ._("type", type->toJson())
      ._("filePlacement", fileRange);
}

} // namespace qat::AST