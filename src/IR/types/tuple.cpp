#include "./tuple.hpp"
#include "qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"
#include <vector>

namespace qat {
namespace IR {

TupleType::TupleType(llvm::LLVMContext &ctx,
                     const std::vector<QatType *> _types, const bool _isPacked)
    : types(_types), isPacked(_isPacked) {
  std::vector<llvm::Type *> subTypes;
  for (auto typ : _types) {
    subTypes.push_back(typ->getLLVMType());
  }
  llvmType = llvm::StructType::get(ctx, subTypes, _isPacked);
}

std::vector<QatType *> TupleType::getSubTypes() const { return types; }

QatType *TupleType::getSubtypeAt(unsigned index) { return types.at(index); }

unsigned TupleType::getSubTypeCount() const { return types.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

TypeKind TupleType::typeKind() const { return TypeKind::tuple; }

} // namespace IR
} // namespace qat