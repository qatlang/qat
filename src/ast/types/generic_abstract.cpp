#include "./generic_abstract.hpp"
#include "qat_type.hpp"

namespace qat::ast {

GenericAbstractType::GenericAbstractType(String _id, String _name, bool isVariable, Maybe<ast::QatType*> _defaultTy,
                                         FileRange _fileRange)
    : QatType(isVariable, std::move(_fileRange)), id(std::move(_id)), name(std::move(_name)), defaultTy(_defaultTy) {
  templates.push_back(this);
}

String GenericAbstractType::getID() const { return id; }

String GenericAbstractType::getName() const { return name; }

bool GenericAbstractType::hasDefault() const { return defaultTy.has_value(); }

Maybe<ast::QatType*> GenericAbstractType::getDefault() const { return defaultTy; }

void GenericAbstractType::setType(IR::QatType* typ) const {
  for (auto* temp : templates) {
    if (temp->id == id) {
      temp->typeValue = typ;
    }
  }
}

bool GenericAbstractType::isSet() const { return (typeValue != nullptr) || (defaultTy.has_value()); }

void GenericAbstractType::unsetType() const {
  for (auto* temp : templates) {
    if (temp->id == id) {
      temp->typeValue = nullptr;
    }
  }
}

String GenericAbstractType::getTemplateID() const { return id; }

String GenericAbstractType::getTemplateName() const { return name; }

TypeKind GenericAbstractType::typeKind() const { return TypeKind::templated; }

IR::QatType* GenericAbstractType::emit(IR::Context* ctx) {
  if (typeValue) {
    return typeValue;
  } else {
    if (hasDefault()) {
      return defaultTy.value()->emit(ctx);
    } else {
      ctx->Error("No type provided for the template type", fileRange);
    }
  }
  return nullptr;
}

Json GenericAbstractType::toJson() const {
  return Json()
      ._("typeKind", "templated")
      ._("id", id)
      ._("name", name)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String GenericAbstractType::toString() const { return "var " + name; }

} // namespace qat::ast