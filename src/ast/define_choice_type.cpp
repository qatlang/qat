#include "./define_choice_type.hpp"
#include "../IR/logic.hpp"
#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

void DefineChoiceType::createType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "choice type", None);
  Vec<Identifier>                fieldNames;
  Maybe<Vec<llvm::ConstantInt*>> fieldValues;
  IR::QatType*                   variantValueType = nullptr;
  Maybe<llvm::ConstantInt*>      lastVal;
  Maybe<IR::QatType*>            providedType;
  if (providedIntegerTy) {
    providedType = providedIntegerTy.value()->emit(ctx);
    if (!providedType.value()->isInteger() && !providedType.value()->isUnsignedInteger()) {
      ctx->Error("Choice types can only have an integer or unsigned integer as its underlying type. This is incorrect",
                 providedIntegerTy.value()->fileRange);
    }
  }
  for (usize i = 0; i < fields.size(); i++) {
    for (usize j = i + 1; j < fields.size(); j++) {
      if (fields.at(j).first.value == fields.at(i).first.value) {
        ctx->Error("Name of the choice variant is repeating here", fields.at(j).first.range);
      }
    }
    fieldNames.push_back(fields.at(i).first);
    if (fields.at(i).second.has_value()) {
      if (providedType && fields.at(i).second.value()->hasTypeInferrance()) {
        fields.at(i).second.value()->asTypeInferrable()->setInferenceType(providedType.value());
      }
      auto iVal   = fields.at(i).second.value()->emit(ctx);
      auto iValTy = iVal->getType();
      if (!iValTy->isInteger() && !iValTy->isUnsignedInteger() &&
          !(iValTy->isCType() &&
            (iValTy->asCType()->getSubType()->isUnsignedInteger() || iValTy->asCType()->getSubType()->isInteger()))) {
        ctx->Error(
            "The value for a variant of this choice type should be of integer or unsigned integer type. Got an expression of type " +
                ctx->highlightError(iValTy->toString()),
            fields.at(i).second.value()->fileRange);
      }
      if (iValTy->isCType()) {
        iValTy = iValTy->asCType()->getSubType();
      }
      if (providedType.has_value() && !iVal->getType()->isSame(providedType.value())) {
        ctx->Error("The provided value of the variant of this choice type has type " +
                       ctx->highlightError(iVal->getType()->toString()) +
                       ", which does not match the provided underlying type " +
                       ctx->highlightError(providedType.value()->toString()),
                   fields.at(i).second.value()->fileRange);
      } else if (!iVal->getType()->isInteger() && !iVal->getType()->isUnsignedInteger()) {
        ctx->Error("Value for variant for choice type should either be a signed or unsigned integer type",
                   fields.at(i).second.value()->fileRange);
      }
      if (variantValueType && !variantValueType->isSame(iVal->getType())) {
        ctx->Error("Value provided for this variant does not match the type of the previous values",
                   fields.at(i).second.value()->fileRange);
      } else if (!variantValueType) {
        variantValueType = iVal->getType();
      }
      llvm::ConstantInt* fieldResult =
          llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(iVal->getLLVMConstant(), ctx->dataLayout.value()));
      if (!lastVal.has_value() && i > 0) {
        Vec<llvm::ConstantInt*> prevVals; // In reverse
        for (usize j = i - 1; j >= 0; j--) {
          if ((prevVals.empty() ? fieldResult : prevVals.back())->isMinValue(iValTy->isInteger()) && j != 0) {
            ctx->Error("Tried to calculate values for variants before " +
                           ctx->highlightError(fields.at(i).first.value) +
                           ". Could not calculate underlying value for the variant " +
                           ctx->highlightError(fields.at(j).first.value) + " as the variant " +
                           ctx->highlightError(fields.at(j + 1).first.value) +
                           " right after this variant has the minimum value possible for the underlying type " +
                           ctx->highlightError(providedType.has_value() ? providedType.value()->toString()
                                                                        : iVal->getType()->toString()) +
                           " of this choice type.",
                       fields.at(j).first.range);
          }
          prevVals.push_back(llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
              llvm::ConstantExpr::getSub(fieldResult,
                                         llvm::ConstantInt::get(fieldResult->getType(), 1u, iValTy->isInteger())),
              ctx->dataLayout.value())));
        }
        fieldValues = Vec<llvm::ConstantInt*>{};
        fieldValues.value().insert(fieldValues.value().end(), prevVals.end(), prevVals.begin());
      }
      lastVal = fieldResult;
      SHOW("Index for field " << fields.at(i).first.value << " is " << *lastVal.value()->getValue().getRawData())
      if (!fieldValues.has_value()) {
        fieldValues = Vec<llvm::ConstantInt*>{};
      }
      SHOW("Pushing field value")
      fieldValues->push_back(lastVal.value());
    } else if (lastVal.has_value()) {
      if (lastVal.value()->isMaxValue(variantValueType->isInteger())) {
        ctx->Error("Could not calculate value for the variant " + ctx->highlightError(fields[i].first.value) +
                       " as the variant " + ctx->highlightError(fields[i - 1].first.value) +
                       " before this has the maximum value possible for the underlying type " +
                       ctx->highlightError(providedType.has_value() ? providedType.value()->toString()
                                                                    : variantValueType->toString()) +
                       " of this choice type",
                   fields[i].first.range);
      }
      SHOW("Last value has value")
      auto newVal = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
          llvm::ConstantExpr::getAdd(
              lastVal.value(), llvm::ConstantInt::get(lastVal.value()->getType(), 1u, variantValueType->isInteger())),
          ctx->dataLayout.value()));
      SHOW("Index for field " << fields.at(i).first.value << " is " << newVal)
      fieldValues->push_back(newVal);
      lastVal = newVal;
    }
  }
  if (fieldValues.has_value()) {
    for (usize i = 0; i < fieldValues->size(); i++) {
      for (usize j = i + 1; j < fieldValues->size(); j++) {
        if (fieldValues->at(i) == fieldValues->at(j)) {
          ctx->Error("Indexing for the field " + ctx->highlightError(fields.at(j).first.value) +
                         " is repeating. Please check logic and make necessary changes",
                     fields.at(j).first.range);
        }
      }
    }
  }
  new IR::ChoiceType(name, mod, std::move(fieldNames), std::move(fieldValues), providedType, areValuesUnsigned, None,
                     ctx->getVisibInfo(visibSpec), ctx, fileRange, None);
}

void DefineChoiceType::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  createType(ctx);
}

Json DefineChoiceType::toJson() const {
  Vec<JsonValue> fieldsJson;
  for (const auto& field : fields) {
    fieldsJson.push_back(
        Json()
            ._("name", field.first)
            ._("hasValue", field.second.has_value())
            ._("value", field.second.has_value() ? field.second.value()->toJson() : JsonValue())
            ._("valueRange", field.second.has_value() ? field.second.value()->fileRange : JsonValue()));
  }
  return Json()
      ._("name", name)
      ._("fields", fieldsJson)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast