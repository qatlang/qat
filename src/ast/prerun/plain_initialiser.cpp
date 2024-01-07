#include "./plain_initialiser.hpp"

namespace qat::ast {

PrerunPlainInit::PrerunPlainInit(Maybe<QatType*> _type, Vec<Pair<String, FileRange>> _fields,
                                 Vec<PrerunExpression*> _fieldValues, FileRange _fileRange)
    : PrerunExpression(_fileRange), type(_type), fields(_fields), fieldValues(_fieldValues) {}

PrerunPlainInit* PrerunPlainInit::Create(Maybe<QatType*> _type, Vec<Pair<String, FileRange>> _fields,
                                         Vec<PrerunExpression*> _fieldValues, FileRange _fileRange) {
  return new PrerunPlainInit(_type, _fields, _fieldValues, _fileRange);
}

IR::PrerunValue* PrerunPlainInit::emit(IR::Context* ctx) {
  auto  reqInfo  = ctx->getAccessInfo();
  auto* typeEmit = type.has_value() ? type.value()->emit(ctx) : inferredType;
  if (type.has_value() && isTypeInferred()) {
    if (!typeEmit->isSame(inferredType)) {
      ctx->Error("The provided type is " + ctx->highlightError(typeEmit->toString()) +
                     " but the type inferred from scope is " + ctx->highlightError(inferredType->toString()),
                 type.value()->fileRange);
    }
  }
  if (typeEmit->isCoreType()) {
    auto cTy = typeEmit->asCore();
    if (cTy->canBePrerun()) {
      std::set<usize>      presentMems;
      Vec<llvm::Constant*> fieldConstants;
      fieldConstants.reserve(cTy->getMemberCount());
      for (usize i = 0; i < fields.size(); i++) {
        auto& field = fields.at(i);
        for (usize k = i + 1; k < fields.size(); k++) {
          if (fields.at(k).first == field.first) {
            ctx->Error("Member field " + ctx->highlightError(field.first) + " is repeating here", fields.at(k).second);
          }
        }
        if (cTy->hasMember(field.first)) {
          auto mem = cTy->getMember(field.first);
          if (!mem->visibility.isAccessible(reqInfo)) {
            ctx->Error("Member field " + ctx->highlightError(field.first) + " is not accessible here", field.second);
          }
          auto fieldVal = fieldValues.at(i)->emit(ctx);
          if (!mem->type->isSame(fieldVal->getType())) {
            ctx->Error("Member field " + ctx->highlightError(field.first) + " is of type " +
                           ctx->highlightError(mem->type->toString()) + " but the expression provided is of type " +
                           ctx->highlightError(fieldVal->getType()->toString()),
                       fieldValues.at(i)->fileRange);
          }
          presentMems.insert(cTy->getMemberIndex(field.first));
          fieldConstants[cTy->getMemberIndex(field.first)] = fieldVal->getLLVMConstant();
        } else {
          ctx->Error("Type " + ctx->highlightError(field.first) + " does not have a member field that is ",
                     field.second);
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
      return new IR::PrerunValue(
          llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(cTy->getLLVMType()), fieldConstants), cTy);
    } else {
      ctx->Error("Type " + ctx->highlightError(cTy->toString()) +
                     " cannot be used in prerun expressions and hence cannot be initialised here",
                 fileRange);
    }
  } else {
    ctx->Error("Type " + ctx->highlightError(typeEmit->toString()) + " cannot be plain initialised", fileRange);
  }
}

Json PrerunPlainInit::toJson() const {
  Vec<JsonValue> fieldsJson;
  Vec<JsonValue> fieldValsJson;
  for (auto& f : fields) {
    fieldsJson.push_back(Json()._("name", f.first)._("range", f.second));
  }
  for (auto f : fieldValues) {
    fieldValsJson.push_back(f->toJson());
  }
  return Json()
      ._("nodeType", "prerunPlainInitialiser")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->toJson() : JsonValue())
      ._("fields", fieldsJson)
      ._("fieldValues", fieldValsJson);
}

} // namespace qat::ast