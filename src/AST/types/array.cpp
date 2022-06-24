#include "./array.hpp"

namespace qat {
namespace AST {

ArrayType::ArrayType(QatType _element_type, const uint64_t _length,
                     const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : element_type(_element_type), length(_length),
      QatType(_variable, _filePlacement) {}

llvm::Type *ArrayType::generate(qat::IR::Generator *generator) {
  return llvm::ArrayType::get(element_type.generate(generator), length);
}

TypeKind ArrayType::typeKind() { return TypeKind::array; }

} // namespace AST
} // namespace qat