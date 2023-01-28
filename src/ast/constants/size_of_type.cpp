#include "./size_of_type.hpp"

namespace qat::ast {

#define LLVM_SIZEOF_RESULT_BITWIDTH 64

SizeOfType::SizeOfType(QatType* _type, FileRange _fileRange) : ConstantExpression(std::move(_fileRange)), type(_type) {}

IR::ConstantValue* SizeOfType::emit(IR::Context* ctx) {
  return new IR::ConstantValue(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx),
                                                      ctx->getMod()->getLLVMModule()->getDataLayout().getTypeStoreSize(
                                                          type->emit(ctx)->getLLVMType())),
                               IR::UnsignedType::get(LLVM_SIZEOF_RESULT_BITWIDTH, ctx->llctx));
}

String SizeOfType::toString() const { return "type'size(" + type->toString() + ")"; }

Json SizeOfType::toJson() const {
  return Json()._("nodeType", "sizeOfType")._("type", type->toJson())._("fileRange", fileRange);
}

} // namespace qat::ast