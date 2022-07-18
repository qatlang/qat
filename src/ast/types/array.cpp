#include "./array.hpp"
#include "../../IR/types/array.hpp"

namespace qat::ast {

ArrayType::ArrayType(QatType *_element_type, const uint64_t _length,
                     const bool _variable, const utils::FileRange _fileRange)
    : element_type(_element_type), length(_length),
      QatType(_variable, _fileRange) {}

IR::QatType *ArrayType::emit(IR::Context *ctx) {
  return new IR::ArrayType(element_type->emit(ctx), length);
}

void ArrayType::emitCPP(backend::cpp::File &file, bool isHeader) const {
  element_type->emitCPP(file, isHeader);
  if (isConstant()) {
    file += " const";
  }
  file += "* ";
}

TypeKind ArrayType::typeKind() { return TypeKind::array; }

nuo::Json ArrayType::toJson() const {
  return nuo::Json()
      ._("typeKind", "array")
      ._("subType", element_type->toJson())
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

std::string ArrayType::toString() const {
  return (isVariable() ? "var " : "") + element_type->toString() + "[" +
         std::to_string(length) + "]";
}

} // namespace qat::ast