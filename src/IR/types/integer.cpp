#include "./integer.hpp"
#include "../../utils/json.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

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

Json IntegerType::toJson() const {
  return Json()._("type", "integer")._("bitWidth", std::to_string(bitWidth));
}

} // namespace qat::IR