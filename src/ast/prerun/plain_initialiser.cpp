#include "./plain_initialiser.hpp"

namespace qat::ast {

PrerunPlainInit::PrerunPlainInit(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields,
                                 Vec<PrerunExpression*> _fieldValues, FileRange _fileRange)
    : PrerunExpression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

PrerunPlainInit* PrerunPlainInit::Create(Maybe<PrerunExpression*> _type, Maybe<Vec<Identifier>> _fields,
                                         Vec<PrerunExpression*> _fieldValues, FileRange _fileRange) {
  return new PrerunPlainInit(_type, _fields, _fieldValues, _fileRange);
}

IR::PrerunValue* PrerunPlainInit::emit(IR::Context* ctx) {
  auto  reqInfo = ctx->getAccessInfo();
  auto* typePreEmit =
      type.has_value() ? type.value()->emit(ctx) : new IR::PrerunValue(IR::TypedType::get(inferredType));
  if (type.has_value() && isTypeInferred()) {
    if (!typePreEmit->getType()->isTyped()) {
      ctx->Error("Expected a type here, but got an expression", type.value()->fileRange);
    }
    if (!typePreEmit->getType()->asTyped()->getSubType()->isSame(inferredType)) {
      ctx->Error("The provided type is " +
                     ctx->highlightError(typePreEmit->getType()->asTyped()->getSubType()->toString()) +
                     " but the type inferred from scope is " + ctx->highlightError(inferredType->toString()),
                 type.value()->fileRange);
    }
  } else if (!type.has_value() && !isTypeInferred()) {
    ctx->Error("No type is provided for plain initialisation and no type could be inferred from scope", fileRange);
  }
  auto typeEmit = typePreEmit->getType()->asTyped()->getSubType();
  if (typeEmit->isCoreType()) {
    auto cTy = typeEmit->asCore();
    if (cTy->canBePrerun()) {
      std::set<usize>      presentMems;
      Vec<llvm::Constant*> fieldConstants;
      fieldConstants.reserve(cTy->getMemberCount());
      if (fields.has_value()) {
        for (usize i = 0; i < fields->size(); i++) {
          auto& field = fields->at(i);
          for (usize k = i + 1; k < fields->size(); k++) {
            if (fields->at(k).value == field.value) {
              ctx->Error("Member field " + ctx->highlightError(field.value) + " is repeating here",
                         fields->at(k).range);
            }
          }
          if (cTy->hasMember(field.value)) {
            auto mem = cTy->getMember(field.value);
            if (!mem->visibility.isAccessible(reqInfo)) {
              ctx->Error("Member field " + ctx->highlightError(field.value) + " is not accessible here", field.range);
            }
            auto fieldVal = fieldValues.at(i)->emit(ctx);
            if (!mem->type->isSame(fieldVal->getType())) {
              ctx->Error("Member field " + ctx->highlightError(field.value) + " is of type " +
                             ctx->highlightError(mem->type->toString()) + " but the expression provided is of type " +
                             ctx->highlightError(fieldVal->getType()->toString()),
                         fieldValues.at(i)->fileRange);
            }
            presentMems.insert(cTy->getMemberIndex(field.value));
            fieldConstants[cTy->getMemberIndex(field.value)] = fieldVal->getLLVMConstant();
          } else {
            ctx->Error("Type " + ctx->highlightError(field.value) + " does not have a member field that is ",
                       field.range);
          }
        }
        Vec<String> missingMembers;
        for (usize i = 0; i < cTy->getMemberCount(); i++) {
          if (!presentMems.contains(i)) {
            missingMembers.push_back(cTy->getMemberNameAt(i));
          }
        }
        if (!missingMembers.empty()) {
          String memMessage;
          for (usize i = 0; i < missingMembers.size(); i++) {
            memMessage += missingMembers[i];
            if (i != missingMembers.size() - 1) {
              if ((missingMembers.size() > 1) && (i == (missingMembers.size() - 2))) {
                memMessage += " and ";
              } else {
                memMessage += ", ";
              }
            }
          }
          ctx->Error("Member fields " + ctx->highlightError(memMessage) + " are missing in this plain initialisation",
                     fileRange);
        }
      } else {
        // FIXME - ?? Change to support default values
        if (cTy->getMemberCount() != fieldValues.size()) {
          ctx->Error("Expected " + ctx->highlightError(std::to_string(cTy->getMemberCount())) +
                         " expressions to provide values for all member fields of type " +
                         ctx->highlightError(cTy->toString()),
                     fileRange);
        }
        for (usize i = 0; i < fieldValues.size(); i++) {
          auto fieldVal = fieldValues.at(i)->emit(ctx);
          if (fieldVal->getType()->isSame(cTy->getMemberAt(i)->type)) {
            fieldConstants.push_back(fieldVal->getLLVMConstant());
          } else {
            ctx->Error(
                "The member field " + ctx->highlightError(cTy->getMemberNameAt(i)) + " expects an expression of type " +
                    ctx->highlightError(cTy->getMemberAt(i)->type->toString()) +
                    " but the provided expression is of type " + ctx->highlightError(fieldVal->getType()->toString()),
                fieldValues.at(i)->fileRange);
          }
        }
      }
      return new IR::PrerunValue(
          llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(cTy->getLLVMType()), fieldConstants), cTy);
    } else {
      ctx->Error("Type " + ctx->highlightError(cTy->toString()) +
                     " cannot be used in prerun expressions and hence cannot be initialised here",
                 fileRange);
    }
  } else {
    ctx->Error("Type " + ctx->highlightError(typeEmit->toString()) +
                   " is not a struct type and hence cannot be plain initialised",
               fileRange);
  }
}

Json PrerunPlainInit::toJson() const {
  Vec<JsonValue> fieldsJson;
  Vec<JsonValue> fieldValsJson;
  if (fields.has_value()) {
    for (auto& f : fields.value()) {
      fieldsJson.push_back(Json()._("name", f.value)._("range", f.range));
    }
  }
  for (auto f : fieldValues) {
    fieldValsJson.push_back(f->toJson());
  }
  return Json()
      ._("nodeType", "prerunPlainInitialiser")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->toJson() : JsonValue())
      ._("hasFields", fields.has_value())
      ._("fields", fieldsJson)
      ._("fieldValues", fieldValsJson);
}

} // namespace qat::ast