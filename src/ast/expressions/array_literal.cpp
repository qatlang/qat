#include "./array_literal.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../IR/types/void.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/unsigned_literal.hpp"
#include "./default.hpp"
#include "./none.hpp"
#include "llvm/IR/Constants.h"

namespace qat::ast {

ArrayLiteral::ArrayLiteral(Vec<Expression*> _values, FileRange _fileRange)
    : Expression(std::move(_fileRange)), values(std::move(_values)), local(nullptr), isVar(false) {}

bool ArrayLiteral::hasLocal() const { return (local != nullptr); }

IR::Value* ArrayLiteral::emit(IR::Context* ctx) {
  IR::ArrayType* arrTyOfLocal = nullptr;
  if (hasLocal()) {
    if (local->getType()->isMaybe()) {
      arrTyOfLocal = local->getType()->asMaybe()->getSubType()->asArray();
    } else {
      arrTyOfLocal = local->getType()->asArray();
    }
  }
  if (hasLocal() && (arrTyOfLocal->getLength() != values.size())) {
    ctx->Error("The expected length of the array is " + ctx->highlightError(std::to_string(arrTyOfLocal->getLength())) +
                   ", but this array literal has " + ctx->highlightError(std::to_string(values.size())) + " elements",
               fileRange);
  } else if (inferredType && inferredType->getLength() != values.size()) {
    auto infLen = inferredType->getLength();
    ctx->Error("The length of the array type inferred is " + ctx->highlightError(std::to_string(infLen)) + ", but " +
                   ((values.size() < infLen) ? "only " : "") + ctx->highlightError(std::to_string(values.size())) +
                   " value" + ((values.size() > 1u) ? "s were" : "was") + " provided.",
               fileRange);
  }
  Vec<IR::Value*> valsIR;
  for (auto* val : values) {
    if (arrTyOfLocal || inferredType) {
      auto* elemTy = arrTyOfLocal ? arrTyOfLocal->getElementType() : inferredType->getElementType();
      switch (val->nodeType()) {
        case NodeType::integerLiteral: {
          ((IntegerLiteral*)val)->setType(elemTy);
          break;
        }
        case NodeType::unsignedLiteral: {
          ((UnsignedLiteral*)val)->setType(elemTy);
          break;
        }
        case NodeType::Default: {
          ((Default*)val)->setType(elemTy);
          break;
        }
        case NodeType::none: {
          ((NoneExpression*)val)->setType(elemTy);
          break;
        }
        default:
          break;
      }
    }
    valsIR.push_back(val->emit(ctx));
  }
  IR::QatType* elemTy = nullptr;
  if (!values.empty()) {
    if (valsIR.front()->isReference()) {
      elemTy = valsIR.front()->getType()->asReference()->getSubType();
    } else {
      elemTy = valsIR.front()->getType();
    }
    for (usize i = 1; i < valsIR.size(); i++) {
      auto* val = valsIR.at(i);
      if (val->getType()->isSame(elemTy) ||
          (val->getType()->isReference() && val->getType()->asReference()->getSubType()->isSame(elemTy))) {
        val->loadImplicitPointer(ctx->builder);
        if (val->getType()->isReference()) {
          valsIR.at(i) = new IR::Value(
              ctx->builder.CreateLoad(val->getType()->asReference()->getSubType()->getLLVMType(), val->getLLVM()),
              val->getType()->asReference()->getSubType(), val->getType()->asReference()->isSubtypeVariable(),
              IR::Nature::temporary);
        }
      } else {
        SHOW("Array element type is: " << val->getType()->toString())
        if (val->getType()->isReference()) {
          SHOW("Array element is a reference to: " << val->getType()->asReference()->getSubType()->toString());
        }
        ctx->Error("The type of this element do not match expected type. The "
                   "expected type is " +
                       ctx->highlightError(elemTy->toString()) + " and the provided type of this element is " +
                       ctx->highlightError(val->getType()->toString()),
                   values.at(i)->fileRange);
      }
    }
    // TODO - Implement constant array literals
    llvm::Value* alloca;
    if (hasLocal()) {
      if (local->getType()->isMaybe()) {
        auto* locll = local->getLLVM();
        alloca      = ctx->builder.CreateStructGEP(local->getType()->getLLVMType(), locll, 1u);
      } else {
        alloca = local->getLLVM();
      }
    } else if (name) {
      auto* loc = ctx->fn->getBlock()->newValue(name->value, IR::ArrayType::get(elemTy, values.size(), ctx->llctx),
                                                isVar, name->range);
      alloca    = loc->getAlloca();
    } else {
      alloca = ctx->builder.CreateAlloca(llvm::ArrayType::get(elemTy->getLLVMType(), values.size()));
    }
    auto* elemPtr =
        ctx->builder.CreateInBoundsGEP(IR::ArrayType::get(elemTy, valsIR.size(), ctx->llctx)->getLLVMType(), alloca,
                                       {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u),
                                        llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)});
    ctx->builder.CreateStore(valsIR.at(0)->getLLVM(), elemPtr);
    for (usize i = 1; i < valsIR.size(); i++) {
      elemPtr = ctx->builder.CreateInBoundsGEP(elemTy->getLLVMType(), elemPtr,
                                               llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 1u));
      SHOW("Value at " << i << " is of type " << valsIR.at(i)->getType()->toString())
      ctx->builder.CreateStore(valsIR.at(i)->getLLVM(), elemPtr);
    }
    return new IR::Value(alloca, IR::ArrayType::get(elemTy, valsIR.size(), ctx->llctx),
                         local ? local->isVariable() : isVar, local ? local->getNature() : IR::Nature::pure);
  } else {
    if (hasLocal()) {
      if (local->getType()->isArray()) {
        ctx->builder.CreateStore(llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(arrTyOfLocal->getLLVMType()), {}),
                                 local->getLLVM());
      }
    } else {
      if (inferredType->getElementType()) {
        return new IR::ConstantValue(
            llvm::ConstantArray::get(llvm::ArrayType::get(inferredType->getElementType()->getLLVMType(), 0u), {}),
            IR::ArrayType::get(inferredType->getElementType(), 0u, ctx->llctx));
      } else {
        ctx->Error("Element type for the empty array is not provided and could not be inferred", fileRange);
      }
    }
    return nullptr;
  }
}

Json ArrayLiteral::toJson() const {
  Vec<JsonValue> vals;
  for (auto* exp : values) {
    vals.push_back(exp->toJson());
  }
  return Json()._("nodeType", "arrayLiteral")._("values", vals);
}

} // namespace qat::ast