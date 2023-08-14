#include "./tuple.hpp"
#include "../../memory_tracker.hpp"
#include "./pointer.hpp"
#include "reference.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

TupleType::TupleType(Vec<QatType*> _types, bool _isPacked, llvm::LLVMContext& llctx)
    : subTypes(std::move(_types)), isPacked(_isPacked) {
  Vec<llvm::Type*> subTypes;
  for (auto* typ : types) {
    subTypes.push_back(typ->getLLVMType());
  }
  llvmType = llvm::StructType::get(llctx, subTypes, isPacked);
}

bool TupleType::isDestructible() const {
  for (auto* sub : subTypes) {
    if (sub->isDestructible()) {
      return true;
    }
  }
  return false;
}

void TupleType::destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) {
  if (isDestructible() && !vals.empty()) {
    auto* instIR = vals.front();
    if (instIR->isReference()) {
      instIR->loadImplicitPointer(ctx->builder);
    }
    for (usize i = 0; i < subTypes.size(); i++) {
      auto* subTy = subTypes.at(i);
      if (subTy->isDestructible() && !subTy->isPointer()) {
        subTy->destroyValue(ctx,
                            {new IR::Value(ctx->builder.CreateStructGEP(llvmType, instIR->getLLVM(), i),
                                           IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary)},
                            fun);
      }
    }
  }
}

TupleType* TupleType::get(Vec<QatType*> newSubTypes, bool isPacked, llvm::LLVMContext& llctx) {
  for (auto* typ : types) {
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

QatType* TupleType::getSubtypeAt(u64 index) { return subTypes.at(index); }

u64 TupleType::getSubTypeCount() const { return subTypes.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

bool TupleType::isTypeSized() const { return !subTypes.empty(); }

TypeKind TupleType::typeKind() const { return TypeKind::tuple; }

String TupleType::toString() const {
  String result = "(";
  for (usize i = 0; i < types.size(); i++) {
    result += types.at(i)->toString();
    if (i != (types.size() - 1)) {
      result += ", ";
    }
  }
  result += ")";
  return result;
}

} // namespace qat::IR