#include "./reference.hpp"

namespace qat {
namespace AST {

ReferenceType::ReferenceType(QatType *_type, bool _variable,
                             utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

llvm::Type *ReferenceType::generate(IR::Generator *generator) {
  return llvm::PointerType::get(type->generate(generator), 0);
}

TypeKind ReferenceType::typeKind() { return TypeKind::reference; }

} // namespace AST
} // namespace qat