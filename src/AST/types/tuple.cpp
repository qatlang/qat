#include "./tuple.hpp"

namespace qat {
namespace AST {

TupleType::TupleType(const std::vector<QatType> _types, const bool _isPacked,
                     const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : types(_types), isPacked(_isPacked), QatType(_variable, _filePlacement) {}

void TupleType::add_type(QatType type) { types.push_back(type); }

llvm::Type *TupleType::generate(IR::Generator *generator) {
  std::vector<llvm::Type *> gen_types;
  for (auto &type : types) {
    llvm::Type *newTy = type.generate(generator);
    if (newTy->isVoidTy()) {
      generator->throw_error("Tuple member type cannot be `void`",
                             filePlacement);
    }
    gen_types.push_back(newTy);
  }
  return llvm::StructType::get(generator->llvmContext,
                               llvm::ArrayRef(gen_types), isPacked);
}

TypeKind TupleType::typeKind() { return TypeKind::tuple; }

} // namespace AST
} // namespace qat