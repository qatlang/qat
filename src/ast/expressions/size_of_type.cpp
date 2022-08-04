#include "./size_of_type.hpp"

namespace qat::ast {

SizeOfType::SizeOfType(QatType *_type, utils::FileRange _fileRange)
    : type(_type), Expression(std::move(_fileRange)) {}

IR::Value *SizeOfType::emit(IR::Context *ctx) {
  // TODO - Implement this
}

nuo::Json SizeOfType::toJson() const {
  return nuo::Json()
      ._("nodeType", "sizeOfType")
      ._("type", type->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast