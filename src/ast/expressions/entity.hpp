#ifndef QAT_AST_EXPRESSIONS_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_ENTITY_HPP

#include "../expression.hpp"
#include <string>

namespace qat::ast {

/**
 *  Entity represents either a variable or constant. The name of the
 * entity is mostly just an identifier, but it can be other values if the
 * entity is present in the global constant
 *
 */
class Entity : public Expression {

private:
  Vec<Identifier> names;
  u32             relative;

  bool canBeChoice = false;

public:
  Entity(u32 relative, Vec<Identifier> _name, FileRange _fileRange);

  void setCanBeChoice();

  useit IR::Value* emit(IR::Context* ctx);
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::entity; }
};

} // namespace qat::ast

#endif