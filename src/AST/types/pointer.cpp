#include "./pointer.hpp"

namespace qat {
namespace AST {

PointerType::PointerType(QatType *_type, const bool _variable,
                         const utils::FilePlacement _filePlacement)
    : type(_type), QatType(_variable, _filePlacement) {}

llvm::Type *PointerType::generate(IR::Generator *generator) {
  SHOW("Before generating type of Pointer")
  auto genType = type->generate(generator)->getPointerTo();
  SHOW("Generated type " << utils::llvmTypeToName(genType))
  return genType;
}

TypeKind PointerType::typeKind() { return TypeKind::pointer; }

} // namespace AST
} // namespace qat