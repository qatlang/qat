#include "./to_conversion.hpp"

namespace qat {
namespace AST {

IR::Value *ToConversion::emit(IR::Context *ctx) {
  // llvm::Value *generatedSource = source->emit(ctx);
  // llvm::Type *sourceType = generatedSource->getType();
  // auto newType = destinationType->emit(ctx)->getLLVMType();
  // if (sourceType->isIntegerTy()) {
  //   if (newType->isIntegerTy()) {
  //     int sourceBitWidth = sourceType->getIntegerBitWidth();
  //     int newBitWidth = newType->getIntegerBitWidth();
  //     if (sourceBitWidth == newBitWidth) {
  //       return generatedSource;
  //     } else if (sourceBitWidth < newBitWidth) {
  //       return ctx->builder.CreateIntCast(generatedSource, newType, true,
  //       "");
  //     } else {

  //       return ctx->builder.CreateBitCast(generatedSource, newType,
  //                                         "int_to_int");
  //     }
  //   } else if (newType->isFloatingPointTy()) {
  //     return ctx->builder.CreateSIToFP(generatedSource, newType,
  //                                      "int_to_float");
  //   }
  //   // TODO: Handle conversion functions
  // }
}

backend::JSON ToConversion::toJSON() const {
  return backend::JSON()
      ._("nodeType", "toConversion")
      ._("instance", source->toJSON())
      ._("targetType", destinationType->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat