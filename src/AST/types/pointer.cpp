#include "./pointer.hpp"

namespace qat {
namespace AST {

PointerType::PointerType(QatType _type, const bool _variable,
                         const utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

llvm::Type *PointerType::generate(IR::Generator *generator) {
  return llvm::PointerType::get(type.generate(generator), 0);
}

TypeKind PointerType::typeKind() { return TypeKind::pointer; }

} // namespace AST
} // namespace qat