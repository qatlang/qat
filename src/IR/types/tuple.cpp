#include "./tuple.hpp"
#include "../../memory_tracker.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

TupleType::TupleType(Vec<QatType*> _types, bool _isPacked, llvm::LLVMContext& ctx)
    : subTypes(std::move(_types)), isPacked(_isPacked) {
  Vec<llvm::Type*> subTypes;
  for (auto* typ : types) {
    subTypes.push_back(typ->getLLVMType());
  }
  llvmType = llvm::StructType::get(ctx, subTypes, isPacked);
}

TupleType* TupleType::get(Vec<QatType*> newSubTypes, bool isPacked, llvm::LLVMContext& ctx) {
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
  return new TupleType(newSubTypes, isPacked, ctx);
}

Vec<QatType*> TupleType::getSubTypes() const { return subTypes; }

QatType* TupleType::getSubtypeAt(u64 index) { return subTypes.at(index); }

u64 TupleType::getSubTypeCount() const { return subTypes.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

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

Json TupleType::toJson() const {
  Vec<JsonValue> jsonValues;
  for (auto* typ : types) {
    jsonValues.push_back(typ->getID());
  }
  return Json()._("type", "tuple")._("subTypes", jsonValues);
}

} // namespace qat::IR