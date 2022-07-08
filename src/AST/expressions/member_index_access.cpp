#include "./member_index_access.hpp"

namespace qat {
namespace AST {

IR::Value *MemberIndexAccess::emit(IR::Context *ctx) {
  // auto instanceValue = instance->emit(ctx);
  // auto indexValue = index->emit(ctx);
  // if (!(indexValue->getType()->isIntegerTy())) {
  //   ctx->throw_error("Expected integer type for index. The "
  //                    "expression is of the wrong type",
  //                    file_placement);
  // }
  // llvm::ConstantInt *constInt =
  // llvm::dyn_cast<llvm::ConstantInt>(indexValue); if (constInt->isNegative())
  // {
  //   ctx->throw_error("Index provided is negative", file_placement);
  // }
  // if (instanceValue->getType()->isStructTy()) {
  //   llvm::StructType *structType =
  //       llvm::dyn_cast<llvm::StructType>(instanceValue->getType());
  //   if (structType->isLiteral()) {
  //     return ctx->builder.CreateStructGEP(instanceValue->getType(),
  //                                         instanceValue,
  //                                         constInt->getZExtValue(), "");
  //   } else {
  //     ctx->throw_error("Cannot access member of a named type by index "
  //                      "Did you mean to use 'memberName?",
  //                      file_placement);
  //   }
  // } else if (instanceValue->getType()->isArrayTy()) {
  //   return ctx->builder.CreateStructGEP(instanceValue->getType(),
  //   instanceValue,
  //                                       constInt->getZExtValue(), "");
  // }
}

void MemberIndexAccess::emitCPP(backend::cpp::File &file, bool isHeader) const {
  if (!isHeader) {
    file += " (";
    instance->emitCPP(file, isHeader);
    file += "[";
    index->emitCPP(file, isHeader);
    file += "]) ";
  }
}

backend::JSON MemberIndexAccess::toJSON() const {
  return backend::JSON()
      ._("nodeType", "memberIndexAccess")
      ._("instance", instance->toJSON())
      ._("index", index->toJSON())
      ._("filePlacement", file_placement);
}

} // namespace AST
} // namespace qat