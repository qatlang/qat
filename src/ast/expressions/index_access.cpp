#include "./index_access.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

namespace qat::ast {

IndexAccess::IndexAccess(Expression *_instance, Expression *_index,
                         utils::FileRange _fileRange)
    : Expression(std::move(_fileRange)), instance(_instance), index(_index) {}

IR::Value *IndexAccess::emit(IR::Context *ctx) {
  auto *inst     = instance->emit(ctx);
  auto *instType = inst->getType();
  auto *ind      = index->emit(ctx);
  auto *indType  = ind->getType();
  inst->loadImplicitPointer(ctx->builder);
  ind->loadImplicitPointer(ctx->builder);
  if (inst->getType()->isReference()) {
    instType = instType->asReference()->getSubType();
  }
  if (ind->getType()->isReference()) {
    indType = indType->asReference()->getSubType();
  }
  // TODO - Update when supporting operators for core types
  if (instType->isPointer() || instType->isArray() || instType->isTuple()) {
    if (ind->getType()->isUnsignedInteger() ||
        (ind->getType()->isReference() &&
         ind->getType()->asReference()->getSubType()->isUnsignedInteger())) {
      if (inst->getType()->isReference()) {
        SHOW("Instance for member access is a Reference to: "
             << inst->getType()->toString())
        ctx->builder.CreateLoad(instType->getLLVMType(), inst->getLLVM());
      }
      if (ind->getType()->isReference()) {
        ctx->builder.CreateLoad(indType->getLLVMType(), ind->getLLVM());
      }
      if (!instType->isTuple()) {
        Vec<llvm::Value *> idxs;
        idxs.push_back(ind->getLLVM());
        return new IR::Value(
            ctx->builder.CreateInBoundsGEP(
                inst->getType()->isPointer()
                    ? inst->getType()->getLLVMType()->getPointerElementType()
                    : inst->getType()->getLLVMType()->getArrayElementType(),
                inst->getLLVM(), idxs),
            IR::ReferenceType::get(
                instType->isPointer()
                    ? instType->asPointer()->getSubType()
                    : (instType->isArray()
                           ? instType->asArray()->getElementType()
                           : instType),
                ctx->llctx),
            inst->isVariable(), inst->getNature());
      } else {
        if (llvm::isa<llvm::ConstantInt>(ind->getLLVM())) {
          // NOLINTBEGIN(clang-analyzer-core.CallAndMessage)
          auto constantIndex =
              *(llvm::dyn_cast<llvm::ConstantInt>(ind->getLLVM())
                    ->getValue()
                    .getRawData());
          // NOLINTEND(clang-analyzer-core.CallAndMessage)
          if (constantIndex < instType->asTuple()->getSubTypeCount()) {
            return new IR::Value(
                ctx->builder.CreateStructGEP(instType->getLLVMType(),
                                             inst->getLLVM(), constantIndex),
                instType->asTuple()->getSubtypeAt(constantIndex),
                inst->isVariable(), inst->getNature());
          } else {
            ctx->Error("The provided index exceeds the maximum value for this "
                       "tuple type. Remember that tuples are zero indexed. The "
                       "index value starts at " +
                           ctx->highlightError("0") + " and ends at " +
                           ctx->highlightError("length - 1"),
                       index->fileRange);
          }
        } else {
          ctx->Error("Tuple members can only be accessed with indices that are "
                     "unsigned integer literals",
                     index->fileRange);
        }
      }
    } else {
      ctx->Error("The index is of type " + ind->getType()->toString() +
                     ". It should be an unsigned integer",
                 index->fileRange);
    }
  } else {
    ctx->Error("The expression cannot be used for index access",
               instance->fileRange);
  }
}

nuo::Json IndexAccess::toJson() const {
  return nuo::Json()
      ._("nodeType", "memberIndexAccess")
      ._("instance", instance->toJson())
      ._("index", index->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast