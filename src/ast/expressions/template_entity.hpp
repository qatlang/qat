#ifndef QAT_AST_EXPRESSIONS_TEMPLATE_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_TEMPLATE_ENTITY_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class TemplateEntity : public Expression {
private:
  u32            relative;
  String         name;
  Vec<QatType *> templateTypes;

public:
  TemplateEntity(u32 _relative, String _name, Vec<QatType *> _templateTypes,
                 utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::templateEntity; }
};

} // namespace qat::ast

#endif