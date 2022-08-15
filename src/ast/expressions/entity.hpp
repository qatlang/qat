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
  String name;
  u32    relative;

public:
  Entity(u32 relative, String _name, utils::FileRange _fileRange);

  useit IR::Value *emit(IR::Context *ctx);
  useit nuo::Json toJson() const;
  useit NodeType  nodeType() const { return NodeType::entity; }
};

} // namespace qat::ast

#endif