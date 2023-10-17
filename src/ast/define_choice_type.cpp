#include "./define_choice_type.hpp"
#include "../IR/logic.hpp"

namespace qat::ast {

DefineChoiceType::DefineChoiceType(Identifier _name, Vec<Pair<Identifier, Maybe<Value>>> _fields,
                                   bool _areValuesUnsigned, Maybe<usize> _defaulVal, VisibilityKind _visibility,
                                   FileRange _fileRange)
    : Node(std::move(_fileRange)), name(std::move(_name)), fields(std::move(_fields)),
      areValuesUnsigned(_areValuesUnsigned), visibility(_visibility), defaultVal(_defaulVal) {}

void DefineChoiceType::createType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  ctx->nameCheckInModule(name, "choice type", None);
  Vec<Identifier> fieldNames;
  Maybe<Vec<i64>> fieldValues;
  Maybe<i64>      lastVal;
  for (usize i = 0; i < fields.size(); i++) {
    for (usize j = i + 1; j < fields.size(); j++) {
      if (fields.at(j).first.value == fields.at(i).first.value) {
        ctx->Error("Name of the choice variant is repeating here", fields.at(j).first.range);
      }
    }
    fieldNames.push_back(fields.at(i).first);
    if (fields.at(i).second.has_value()) {
      auto iVal = fields.at(i).second.value().data;
      lastVal   = iVal;
      SHOW("Index for field " << fields.at(i).first.value << " is " << iVal)
      if (!fieldValues.has_value()) {
        fieldValues = Vec<i64>{};
      }
      fieldValues->push_back(iVal);
    } else if (lastVal.has_value()) {
      auto newVal = lastVal.value() + 1;
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
                         " is repeating. Please check logic and make "
                         "necessary changes",
                     fields.at(j).first.range);
        }
      }
    }
  }
  new IR::ChoiceType(name, mod, std::move(fieldNames), std::move(fieldValues), areValuesUnsigned, None,
                     ctx->getVisibInfo(visibility), ctx->llctx, fileRange);
}

void DefineChoiceType::defineType(IR::Context* ctx) {
  auto* mod = ctx->getMod();
  createType(ctx);
}

Json DefineChoiceType::toJson() const {
  Vec<JsonValue> fieldsJson;
  for (const auto& field : fields) {
    fieldsJson.push_back(Json()
                             ._("name", field.first)
                             ._("hasValue", field.second.has_value())
                             ._("value", field.second.has_value() ? field.second->data : JsonValue())
                             ._("valueRange", field.second.has_value() ? field.second->range : JsonValue()));
  }
  return Json()
      ._("name", name)
      ._("fields", fieldsJson)
      ._("visibility", kindToJsonValue(visibility))
      ._("fileRange", fileRange);
}

} // namespace qat::ast