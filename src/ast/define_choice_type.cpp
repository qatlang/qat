#include "./define_choice_type.hpp"
#include "../IR/logic.hpp"
#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "llvm/Analysis/ConstantFolding.h"

namespace qat::ast {

DefineChoiceType::DefineChoiceType(Identifier _name, Vec<Pair<Identifier, Maybe<PrerunExpression*>>> _fields,
                                   Maybe<ast::QatType*> _providedTy, bool _areValuesUnsigned, Maybe<usize> _defaulVal,
                                   Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), fields(std::move(_fields)),
      areValuesUnsigned(_areValuesUnsigned), visibSpec(_visibSpec), defaultVal(_defaulVal),
      providedIntegerTy(_providedTy) {}

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
      auto iVal = fields.at(i).second.value()->emit(ctx);
      if (providedType && !iVal->getType()->isSame(providedType.value())) {
        ctx->Error("The provided value of the variant of this choice type has type " +
                       ctx->highlightError(iVal->getType()->toString()) +
                       ", which does not match the provided underlying type " +
                       ctx->highlightError(providedType.value()->toString()),
                   fields.at(i).second.value()->fileRange);
      } else if (!iVal->getType()->isInteger() && !iVal->getType()->isUnsignedInteger()) {
        ctx->Error("Value for variant for choice type should either be a signed or unsigned integer type",
                   fields.at(i).second.value()->fileRange);
      }
      if (variantValueType && variantValueType->isSame(iVal->getType())) {
        ctx->Error("Value provided for this variant does not match the type of the previous values",
                   fields.at(i).second.value()->fileRange);
      }
      variantValueType = iVal->getType();
      lastVal =
          llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(iVal->getLLVMConstant(), ctx->dataLayout.value()));
      SHOW("Index for field " << fields.at(i).first.value << " is " << iVal)
      if (!fieldValues.has_value()) {
        fieldValues = {};
      }
      fieldValues->push_back(
          llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(iVal->getLLVMConstant(), ctx->dataLayout.value())));
    } else if (lastVal.has_value()) {
      auto newVal = llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldConstant(
          llvm::ConstantExpr::getAdd(lastVal.value(), llvm::ConstantInt::get(variantValueType->getLLVMType(), 1u,
                                                                             variantValueType->isInteger())),
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