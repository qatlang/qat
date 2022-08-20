#include "./array_literal.hpp"

namespace qat::ast {

ArrayLiteral::ArrayLiteral(Vec<Expression *> _values,
                           utils::FileRange  _fileRange)
    : Expression(std::move(_fileRange)), values(std::move(_values)),
      local(nullptr), isVar(false) {}

bool ArrayLiteral::hasLocal() const { return (local != nullptr); }

IR::Value *ArrayLiteral::emit(IR::Context *ctx) {
  if (hasLocal() &&
      (local->getType()->asArray()->getLength() != values.size())) {
    ctx->Error("The expected length of the array is " +
                   ctx->highlightError(std::to_string(
                       local->getType()->asArray()->getLength())) +
                   ", but this array literal has " +
                   ctx->highlightError(std::to_string(values.size())) +
                   " elements",
               fileRange);
  }
  Vec<IR::Value *> valsIR;
  for (auto *val : values) {
    valsIR.push_back(val->emit(ctx));
  }
  IR::QatType *elemTy = nullptr;
  if (!values.empty()) {
    if (valsIR.front()->isReference()) {
      elemTy = valsIR.front()->getType()->asReference()->getSubType();
    } else {
      elemTy = valsIR.front()->getType();
    }
    for (usize i = 1; i < valsIR.size(); i++) {
      auto *val = valsIR.at(i);
      if (val->getType()->isSame(elemTy) ||
          (val->getType()->isReference() &&
           val->getType()->asReference()->getSubType()->isSame(elemTy))) {
        val->loadImplicitPointer(ctx->builder);
        if (val->getType()->isReference()) {
          valsIR.at(i) = new IR::Value(
              ctx->builder.CreateLoad(
                  val->getType()->asReference()->getSubType()->getLLVMType(),
                  val->getLLVM()),
              val->getType()->asReference()->getSubType(),
              val->getType()->asReference()->isSubtypeVariable(),
              IR::Nature::temporary);
        }
      } else {
        SHOW("Array element type is: " << val->getType()->toString())
        if (val->getType()->isReference()) {
          SHOW("Array element is a reference to: "
               << val->getType()->asReference()->getSubType()->toString());
        }
        ctx->Error("The type of this element do not match expected type. The "
                   "expected type is " +
                       ctx->highlightError(elemTy->toString()) +
                       " and the provided type of this element is " +
                       ctx->highlightError(val->getType()->toString()),
                   values.at(i)->fileRange);
      }
    }
    // TODO - Implement constant array literals
    llvm::AllocaInst *alloca;
    if (hasLocal()) {
      alloca = local->getAlloca();
    } else if (!name.empty()) {
      auto *loc = ctx->fn->getBlock()->newValue(
          name, IR::ArrayType::get(elemTy, values.size(), ctx->llctx), isVar);
      alloca = loc->getAlloca();
    } else {
      alloca = ctx->builder.CreateAlloca(
          llvm::ArrayType::get(elemTy->getLLVMType(), values.size()));
    }
    auto *elemPtr = ctx->builder.CreateInBoundsGEP(
        IR::ArrayType::get(elemTy, valsIR.size(), ctx->llctx)->getLLVMType(),
        alloca,
        {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u),
         llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)});
    ctx->builder.CreateStore(valsIR.at(0)->getLLVM(), elemPtr);
    for (usize i = 1; i < valsIR.size(); i++) {
      elemPtr = ctx->builder.CreateInBoundsGEP(
          elemTy->getLLVMType(), elemPtr,
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u));
      SHOW("Value at " << i << " is of type "
                       << valsIR.at(i)->getType()->toString())
      ctx->builder.CreateStore(valsIR.at(i)->getLLVM(), elemPtr);
    }
    return new IR::Value(alloca,
                         IR::ArrayType::get(elemTy, valsIR.size(), ctx->llctx),
                         local ? local->isVariable() : isVar,
                         local ? local->getNature() : IR::Nature::pure);
  }
}

nuo::Json ArrayLiteral::toJson() const {
  Vec<nuo::JsonValue> vals;
  for (auto *exp : values) {
    vals.push_back(exp->toJson());
  }
  return nuo::Json()._("nodeType", "arrayLiteral")._("values", vals);
}

} // namespace qat::ast