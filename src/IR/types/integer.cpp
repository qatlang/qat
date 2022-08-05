#include "./integer.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <nuo/json.hpp>

namespace qat::IR {

IntegerType::IntegerType(u64 _bitWidth, llvm::LLVMContext &ctx)
    : bitWidth(_bitWidth) {
  llvmType = llvm::IntegerType::get(ctx, bitWidth);
}

IntegerType *IntegerType::get(u64 bits, llvm::LLVMContext &ctx) {
  for (auto *typ : types) {
    if (typ->isInteger()) {
      if (typ->asInteger()->isBitWidth(bits)) {
        return typ->asInteger();
      }
    }
  }
  return new IntegerType(bits, ctx);
}

bool IntegerType::isBitWidth(u64 width) const { return bitWidth == width; }

TypeKind IntegerType::typeKind() const { return TypeKind::integer; }

u64 IntegerType::getBitwidth() const { return bitWidth; }

String IntegerType::toString() const { return "i" + std::to_string(bitWidth); }

nuo::Json IntegerType::toJson() const {
  return nuo::Json()
      ._("type", "integer")
      ._("bitWidth", std::to_string(bitWidth));
}

} // namespace qat::IR