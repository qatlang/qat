#ifndef QAT_AST_EXPRESSIONS_TEMPLATE_NAMED_TYPE_HPP
#define QAT_AST_EXPRESSIONS_TEMPLATE_NAMED_TYPE_HPP

#include "../types/qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

class TemplateNamedType : public QatType {
private:
  u32             relative;
  Vec<Identifier> names;
  Vec<QatType*>   templateTypes;

public:
  TemplateNamedType(u32 _relative, Vec<Identifier> names, Vec<QatType*> _templateTypes, bool isVariable,
                    FileRange _fileRange);

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit Json         toJson() const final;
  useit String       toString() const final;
  useit TypeKind     typeKind() const final { return TypeKind::templateNamed; }
};

} // namespace qat::ast

#endif