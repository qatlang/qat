#ifndef QAT_AST_CONSTANTS_ENTITY_HPP
#define QAT_AST_CONSTANTS_ENTITY_HPP

#include "../expression.hpp"
#include "member_access.hpp"

namespace qat::ast {

class PrerunEntity : public PrerunExpression {
  friend class PrerunMemberAccess;

private:
  u32             relative;
  Vec<Identifier> identifiers;

  bool canBeChoice = false;
  bool isChoice    = false;

public:
  PrerunEntity(u32 relative, Vec<Identifier> _ids, FileRange _fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::prerunEntity; }
};

} // namespace qat::ast

#endif