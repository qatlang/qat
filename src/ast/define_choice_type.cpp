#include "./define_choice_type.hpp"

namespace qat::ast {

DefineChoiceType::DefineChoiceType(String                         _name,
                                   Vec<Pair<Field, Maybe<Value>>> _fields,
                                   utils::VisibilityKind          _visibility,
                                   utils::FileRange               _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)),
      fields(std::move(_fields)), visibility(_visibility) {}

void DefineChoiceType::createType(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if (!mod->hasTemplateCoreType(name) && !mod->hasCoreType(name) &&
      !mod->hasFunction(name) && !mod->hasGlobalEntity(name) &&
      !mod->hasBox(name) && !mod->hasTypeDef(name) && !mod->hasMixType(name) &&
      !mod->hasChoiceType(name)) {
    Vec<String>     fieldNames;
    Maybe<Vec<i64>> fieldValues;
    Maybe<i64>      lastVal;
    for (usize i = 0; i < fields.size(); i++) {
      for (usize j = i + 1; j < fields.size(); j++) {
        if (fields.at(j).first.name == fields.at(i).first.name) {
          ctx->Error("Name of the choice variant is repeating here",
                     fields.at(j).first.range);
        }
      }
      fieldNames.push_back(fields.at(i).first.name);
      if (fields.at(i).second.has_value()) {
        auto iVal = fields.at(i).second.value().data;
        lastVal   = iVal;
        SHOW("Index for field " << fields.at(i).first.name << " is " << iVal)
        if (!fieldValues.has_value()) {
          fieldValues = Vec<i64>{};
        }
        fieldValues->push_back(iVal);
      } else if (lastVal.has_value()) {
        auto newVal = lastVal.value() + 1;
        SHOW("Index for field " << fields.at(i).first.name << " is " << newVal)
        fieldValues->push_back(newVal);
        lastVal = newVal;
      }
    }
    if (fieldValues.has_value()) {
      for (usize i = 0; i < fieldValues->size(); i++) {
        for (usize j = i + 1; j < fieldValues->size(); j++) {
          if (fieldValues->at(i) == fieldValues->at(j)) {
            ctx->Error("Indexing for the field " +
                           ctx->highlightError(fields.at(j).first.name) +
                           " is repeating. Please check logic and make "
                           "necessary changes",
                       fields.at(j).first.range);
          }
        }
      }
    }
    new IR::ChoiceType(name, mod, std::move(fieldNames), std::move(fieldValues),
                       ctx->getVisibInfo(visibility), ctx->llctx);
  } else {
    if (mod->hasTemplateCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing template core type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing core type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasTypeDef(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing type definition in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing mix type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing function in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing global value in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing box in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing choice type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    }
  }
}

void DefineChoiceType::defineType(IR::Context *ctx) {
  auto *mod = ctx->getMod();
  if (!mod->hasTemplateCoreType(name) && !mod->hasCoreType(name) &&
      !mod->hasFunction(name) && !mod->hasGlobalEntity(name) &&
      !mod->hasBox(name) && !mod->hasTypeDef(name) && !mod->hasMixType(name) &&
      !mod->hasChoiceType(name)) {
    createType(ctx);
  } else {
    // TODO - Check type definitions
    if (mod->hasTemplateCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing template core type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasCoreType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing core type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasTypeDef(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing type definition in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasMixType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing mix type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasFunction(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing function in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasGlobalEntity(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing global value in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasBox(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing box in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    } else if (mod->hasChoiceType(name)) {
      ctx->Error(
          ctx->highlightError(name) +
              " is the name of an existing choice type in this scope. "
              "Please change name of this choice type or check the codebase",
          fileRange);
    }
  }
}

Json DefineChoiceType::toJson() const {
  Vec<JsonValue> fieldsJson;
  for (const auto &field : fields) {
    fieldsJson.push_back(
        Json()
            ._("name", field.first.name)
            ._("nameRange", field.first.range)
            ._("hasValue", field.second.has_value())
            ._("value",
               field.second.has_value() ? field.second->data : JsonValue())
            ._("valueRange",
               field.second.has_value() ? field.second->range : JsonValue()));
  }
  return Json()
      ._("name", name)
      ._("fields", fieldsJson)
      ._("visibility", utils::kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast