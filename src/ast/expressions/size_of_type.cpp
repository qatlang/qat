#include "./size_of_type.hpp"

namespace qat::ast {

#define LLVM_SIZEOF_RESULT_BITWIDTH 64

SizeOfType::SizeOfType(QatType *_type, utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), type(_type) {}

IR::Value *SizeOfType::emit(IR::Context *ctx) {
  return new IR::Value(
      llvm::ConstantExpr::getSizeOf(type->emit(ctx)->getLLVMType()),
      IR::UnsignedType::get(LLVM_SIZEOF_RESULT_BITWIDTH, ctx->llctx), false,
      IR::Nature::pure);
}

nuo::Json SizeOfType::toJson() const {
  return nuo::Json()
      ._("nodeType", "sizeOfType")
      ._("type", type->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast