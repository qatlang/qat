#ifndef QAT_AST_CONSTANTS_ENTITY_HPP
#define QAT_AST_CONSTANTS_ENTITY_HPP

#include "../expression.hpp"

namespace qat::ast {

class ConstantEntity : public ConstantExpression {
private:
  u32             relative;
  Vec<Identifier> identifiers;

public:
  ConstantEntity(u32 relative, Vec<Identifier> _ids, FileRange _fileRange);

  useit IR::ConstantValue* emit(IR::Context* ctx) override;
  useit Json               toJson() const override;
  useit String             toString() const final;
  useit NodeType           nodeType() const override { return NodeType::constantEntity; }
};

} // namespace qat::ast

#endif