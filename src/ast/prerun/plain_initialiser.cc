#include "./plain_initialiser.hpp"

namespace qat::ast {

ir::PrerunValue* PrerunPlainInit::emit(EmitCtx* ctx) {
  auto  reqInfo = ctx->get_access_info();
  auto* typePreEmit =
      type.has_value() ? type.value()->emit(ctx) : ir::PrerunValue::get_typed_prerun(ir::TypedType::get(inferredType));
  if (type.has_value() && isTypeInferred()) {
    if (!typePreEmit->get_ir_type()->is_typed()) {
      ctx->Error("Expected a type here, but got an expression", type.value()->fileRange);
    }
    if (!typePreEmit->get_ir_type()->as_typed()->get_subtype()->is_same(inferredType)) {
      ctx->Error("The provided type is " +
                     ctx->color(typePreEmit->get_ir_type()->as_typed()->get_subtype()->to_string()) +
                     " but the type inferred from scope is " + ctx->color(inferredType->to_string()),
                 type.value()->fileRange);
    }
  } else if (!type.has_value() && !isTypeInferred()) {
    ctx->Error("No type is provided for plain initialisation and no type could be inferred from scope", fileRange);
  }
  auto typeEmit = typePreEmit->get_ir_type()->as_typed()->get_subtype();
  if (typeEmit->is_struct()) {
    auto cTy = typeEmit->as_struct();
    if (cTy->can_be_prerun()) {
      std::set<usize>      presentMems;
      Vec<llvm::Constant*> fieldConstants;
      fieldConstants.reserve(cTy->get_field_count());
      if (fields.has_value()) {
        for (usize i = 0; i < fields->size(); i++) {
          auto& field = fields->at(i);
          for (usize k = i + 1; k < fields->size(); k++) {
            if (fields->at(k).value == field.value) {
              ctx->Error("Member field " + ctx->color(field.value) + " is repeating here", fields->at(k).range);
            }
          }
          if (cTy->has_field_with_name(field.value)) {
            auto mem = cTy->get_field_with_name(field.value);
            if (!mem->visibility.is_accessible(reqInfo)) {
              ctx->Error("Member field " + ctx->color(field.value) + " is not accessible here", field.range);
            }
            auto fieldVal = fieldValues.at(i)->emit(ctx);
            if (!mem->type->is_same(fieldVal->get_ir_type())) {
              ctx->Error("Member field " + ctx->color(field.value) + " is of type " +
                             ctx->color(mem->type->to_string()) + " but the expression provided is of type " +
                             ctx->color(fieldVal->get_ir_type()->to_string()),
                         fieldValues.at(i)->fileRange);
            }
            presentMems.insert(cTy->get_field_index(field.value));
            fieldConstants[cTy->get_field_index(field.value)] = fieldVal->get_llvm_constant();
          } else {
            ctx->Error("Type " + ctx->color(field.value) + " does not have a member field that is ", field.range);
          }
        }
        Vec<String> missingMembers;
        for (usize i = 0; i < cTy->get_field_count(); i++) {
          if (!presentMems.contains(i)) {
            missingMembers.push_back(cTy->get_field_name_at(i));
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
          ctx->Error("Member fields " + ctx->color(memMessage) + " are missing in this plain initialisation",
                     fileRange);
        }
      } else {
        // FIXME - ?? Change to support default values
        if (cTy->get_field_count() != fieldValues.size()) {
          ctx->Error("Expected " + ctx->color(std::to_string(cTy->get_field_count())) +
                         " expressions to provide values for all member fields of type " + ctx->color(cTy->to_string()),
                     fileRange);
        }
        for (usize i = 0; i < fieldValues.size(); i++) {
          auto fieldVal = fieldValues.at(i)->emit(ctx);
          if (fieldVal->get_ir_type()->is_same(cTy->get_field_at(i)->type)) {
            fieldConstants.push_back(fieldVal->get_llvm_constant());
          } else {
            ctx->Error("The member field " + ctx->color(cTy->get_field_name_at(i)) + " expects an expression of type " +
                           ctx->color(cTy->get_field_at(i)->type->to_string()) +
                           " but the provided expression is of type " +
                           ctx->color(fieldVal->get_ir_type()->to_string()),
                       fieldValues.at(i)->fileRange);
          }
        }
      }
      return ir::PrerunValue::get(
          llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(cTy->get_llvm_type()), fieldConstants), cTy);
    } else {
      ctx->Error("Type " + ctx->color(cTy->to_string()) +
                     " cannot be used in prerun expressions and hence cannot be initialised here",
                 fileRange);
    }
  } else {
    ctx->Error("Type " + ctx->color(typeEmit->to_string()) +
                   " is not a struct type and hence cannot be plain initialised",
               fileRange);
  }
}

String PrerunPlainInit::to_string() const {
  String result;
  if (type.has_value()) {
    result += type.value()->to_string() + " ";
  }
  result += "from { ";
  for (usize i = 0; i < fieldValues.size(); i++) {
    if (fields.has_value()) {
      result += fields.value()[i].value + " := ";
    }
    result += fieldValues[i]->to_string();
    if (i != (fieldValues.size() - 1)) {
      result += ", ";
    }
  }
  result += " }";
  return result;
}

Json PrerunPlainInit::to_json() const {
  Vec<JsonValue> fieldsJson;
  Vec<JsonValue> fieldValsJson;
  if (fields.has_value()) {
    for (auto& f : fields.value()) {
      fieldsJson.push_back(Json()._("name", f.value)._("range", f.range));
    }
  }
  for (auto f : fieldValues) {
    fieldValsJson.push_back(f->to_json());
  }
  return Json()
      ._("nodeType", "prerunPlainInitialiser")
      ._("hasType", type.has_value())
      ._("type", type.has_value() ? type.value()->to_json() : JsonValue())
      ._("hasFields", fields.has_value())
      ._("fields", fieldsJson)
      ._("fieldValues", fieldValsJson);
}

} // namespace qat::ast