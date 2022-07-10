#include "./array.hpp"
#include "../../IR/types/array.hpp"

namespace qat::AST {

ArrayType::ArrayType(QatType *_element_type, const uint64_t _length,
                     const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : element_type(_element_type), length(_length),
      QatType(_variable, _filePlacement) {}

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

backend::JSON ArrayType::toJSON() const {
  return backend::JSON()
      ._("typeKind", "array")
      ._("subType", element_type->toJSON())
      ._("isVariable", isVariable())
      ._("filePlacement", filePlacement);
}

std::string ArrayType::toString() const {
  return (isVariable() ? "var " : "") + element_type->toString() + "[" +
         std::to_string(length) + "]";
}

} // namespace qat::AST