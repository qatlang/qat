#include "./array.hpp"

namespace qat {
namespace AST {

ArrayType::ArrayType(QatType *_element_type, const uint64_t _length,
                     const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : element_type(_element_type), length(_length),
      QatType(_variable, _filePlacement) {}

llvm::Type *ArrayType::emit(qat::IR::Generator *generator) {
  return llvm::ArrayType::get(element_type->emit(generator), length);
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

} // namespace AST
} // namespace qat