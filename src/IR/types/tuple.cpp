#include "./tuple.hpp"
#include "../../memory_tracker.hpp"
#include "./pointer.hpp"
#include "reference.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

TupleType::TupleType(Vec<QatType*> _types, bool _isPacked, llvm::LLVMContext& llctx)
    : subTypes(std::move(_types)), isPacked(_isPacked) {
  Vec<llvm::Type*> subTypesLLVM;
  for (auto* typ : subTypes) {
    subTypesLLVM.push_back(typ->getLLVMType());
  }
  llvmType    = llvm::StructType::get(llctx, subTypesLLVM, isPacked);
  linkingName = "qat'tuple:[" + String(isPacked ? "pack," : "");
  for (usize i = 0; i < subTypes.size(); i++) {
    linkingName += subTypes.at(i)->getNameForLinking();
    if (i != (subTypes.size() - 1)) {
      linkingName += ",";
    }
  }
  linkingName += "]";
}

bool TupleType::isCopyConstructible() const {
  for (auto* sub : subTypes) {
    if (!sub->isCopyConstructible()) {
      return false;
    }
  }
  return true;
}

bool TupleType::isCopyAssignable() const {
  for (auto* sub : subTypes) {
    if (!sub->isCopyAssignable()) {
      return false;
    }
  }
  return true;
}

bool TupleType::isMoveConstructible() const {
  for (auto* sub : subTypes) {
    if (!sub->isMoveConstructible()) {
      return false;
    }
  }
  return true;
}

bool TupleType::isMoveAssignable() const {
  for (auto* sub : subTypes) {
    if (!sub->isMoveAssignable()) {
      return false;
    }
  }
  return true;
}

bool TupleType::isDestructible() const {
  for (auto* sub : subTypes) {
    if (!sub->isDestructible()) {
      return false;
    }
  }
  return true;
}

void TupleType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyCopyable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(llvmType, second->getLLVM()), first->getLLVM());
  } else {
    if (isCopyConstructible()) {
      for (usize i = 0; i < subTypes.size(); i++) {
        auto* subTy = subTypes.at(i);
        subTy->copyConstructValue(ctx,
                                  new IR::Value(ctx->builder.CreateStructGEP(llvmType, first->getLLVM(), i),
                                                IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                                  new IR::Value(ctx->builder.CreateStructGEP(llvmType, second->getLLVM(), i),
                                                IR::ReferenceType::get(false, subTy, ctx), false,
                                                IR::Nature::temporary),
                                  fun);
      }
    } else {
      ctx->Error("Could not copy construct an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

void TupleType::copyAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyCopyable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(llvmType, second->getLLVM()), first->getLLVM());
  } else {
    if (isCopyAssignable()) {
      for (usize i = 0; i < subTypes.size(); i++) {
        auto* subTy = subTypes.at(i);
        subTy->copyAssignValue(ctx,
                               new IR::Value(ctx->builder.CreateStructGEP(llvmType, first->getLLVM(), i),
                                             IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                               new IR::Value(ctx->builder.CreateStructGEP(llvmType, second->getLLVM(), i),
                                             IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                               fun);
      }
    } else {
      ctx->Error("Could not copy assign an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

void TupleType::moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyMovable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(llvmType, second->getLLVM()), first->getLLVM());
    ctx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), second->getLLVM());
  } else {
    if (isMoveConstructible()) {
      for (usize i = 0; i < subTypes.size(); i++) {
        auto* subTy = subTypes.at(i);
        subTy->moveConstructValue(ctx,
                                  new IR::Value(ctx->builder.CreateStructGEP(llvmType, first->getLLVM(), i),
                                                IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                                  new IR::Value(ctx->builder.CreateStructGEP(llvmType, second->getLLVM(), i),
                                                IR::ReferenceType::get(false, subTy, ctx), false,
                                                IR::Nature::temporary),
                                  fun);
      }
    } else {
      ctx->Error("Could not move construct an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

void TupleType::moveAssignValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isTriviallyMovable()) {
    ctx->builder.CreateStore(ctx->builder.CreateLoad(llvmType, second->getLLVM()), first->getLLVM());
    ctx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), second->getLLVM());
  } else {
    if (isMoveAssignable()) {
      for (usize i = 0; i < subTypes.size(); i++) {
        auto* subTy = subTypes.at(i);
        subTy->moveAssignValue(ctx,
                               new IR::Value(ctx->builder.CreateStructGEP(llvmType, first->getLLVM(), i),
                                             IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
                               new IR::Value(ctx->builder.CreateStructGEP(llvmType, second->getLLVM(), i),
                                             IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                               fun);
      }
    } else {
      ctx->Error("Could not move assign an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

void TupleType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  if (isTriviallyMovable()) {
    ctx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), instance->getLLVM());
  } else {
    if (isDestructible()) {
      for (usize i = 0; i < subTypes.size(); i++) {
        auto* subTy = subTypes.at(i);
        subTy->destroyValue(ctx,
                            new IR::Value(ctx->builder.CreateStructGEP(llvmType, instance->getLLVM(), i),
                                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
                            fun);
      }
    } else {
      ctx->Error("Could not destroy an instance of type " + ctx->highlightError(toString()), None);
    }
  }
}

TupleType* TupleType::get(Vec<QatType*> newSubTypes, bool isPacked, llvm::LLVMContext& llctx) {
  for (auto* typ : allQatTypes) {
    if (typ->isTuple()) {
      auto subTys = typ->asTuple()->getSubTypes();
      bool isSame = true;
      if (typ->asTuple()->isPackedTuple() != isPacked) {
        isSame = false;
      } else if (newSubTypes.size() != subTys.size()) {
        isSame = false;
      } else {
        for (usize i = 0; i < subTys.size(); i++) {
          if (!subTys.at(i)->isSame(newSubTypes.at(i))) {
            isSame = false;
            break;
          }
        }
      }
      if (isSame) {
        return typ->asTuple();
      }
    }
  }
  return new TupleType(newSubTypes, isPacked, llctx);
}

Vec<QatType*> TupleType::getSubTypes() const { return subTypes; }

QatType* TupleType::getSubtypeAt(u32 index) { return subTypes.at(index); }

u32 TupleType::getSubTypeCount() const { return subTypes.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

bool TupleType::isTypeSized() const { return !subTypes.empty(); }

TypeKind TupleType::typeKind() const { return TypeKind::tuple; }

String TupleType::toString() const {
  String result = String(isPacked ? "pack " : "") + "(";
  for (usize i = 0; i < subTypes.size(); i++) {
    result += subTypes.at(i)->toString();
    if (i != (subTypes.size() - 1)) {
      result += "; ";
    }
  }
  result += ")";
  return result;
}

} // namespace qat::IR