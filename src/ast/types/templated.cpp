#include "./templated.hpp"
#include "qat_type.hpp"

namespace qat::ast {

TemplatedType::TemplatedType(String _id, String _name, bool isVariable,
                             utils::FileRange _fileRange)
    : QatType(isVariable, std::move(_fileRange)), id(std::move(_id)),
      name(std::move(_name)) {
  templates.push_back(this);
}

String TemplatedType::getID() const { return id; }

String TemplatedType::getName() const { return name; }

void TemplatedType::setType(IR::QatType *typ) const {
  for (auto *temp : templates) {
    if (temp->id == id) {
      temp->typeValue = typ;
    }
  }
}

bool TemplatedType::isSet() const { return typeValue != nullptr; }

void TemplatedType::unsetType() const {
  for (auto *temp : templates) {
    if (temp->id == id) {
      temp->typeValue = nullptr;
    }
  }
}

String TemplatedType::getTemplateID() const { return id; }

String TemplatedType::getTemplateName() const { return name; }

TypeKind TemplatedType::typeKind() const { return TypeKind::templated; }

IR::QatType *TemplatedType::emit(IR::Context *ctx) {
  if (typeValue) {
    return typeValue;
  } else {
    ctx->Error("No type provided for the template type", fileRange);
  }
  return nullptr;
}

nuo::Json TemplatedType::toJson() const {
  return nuo::Json()
      ._("typeKind", "templated")
      ._("id", id)
      ._("name", name)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String TemplatedType::toString() const { return "var " + name; }

} // namespace qat::ast