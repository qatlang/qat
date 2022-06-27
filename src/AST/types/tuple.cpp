#include "./tuple.hpp"
#include <vector>

namespace qat {
namespace AST {

TupleType::TupleType(const std::vector<QatType *> _types, const bool _isPacked,
                     const bool _variable,
                     const utils::FilePlacement _filePlacement)
    : types(_types), isPacked(_isPacked), QatType(_variable, _filePlacement) {}

void TupleType::add_type(QatType *type) { types.push_back(type); }

llvm::Type *TupleType::generate(IR::Generator *generator) {
  std::vector<llvm::Type *> gen_types;
  for (auto &type : types) {
    llvm::Type *newTy = type->generate(generator);
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

backend::JSON TupleType::toJSON() const {
  std::vector<backend::JSON> mems;
  for (auto mem : types) {
    mems.push_back(mem->toJSON());
  }
  return backend::JSON()
      ._("typeKind", "tuple")
      ._("members", mems)
      ._("isVariable", isVariable())
      ._("isPacked", isPacked)
      ._("filePlacement", filePlacement);
}

} // namespace AST
} // namespace qat