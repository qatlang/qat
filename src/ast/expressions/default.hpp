#ifndef QAT_AST_EXPRESSIONS_DEFAULT_HPP
#define QAT_AST_EXPRESSIONS_DEFAULT_HPP

#include "../expression.hpp"

namespace qat::ast {

class Default : public Expression {
  friend class LocalDeclaration;

private:
  Maybe<Identifier> irName;
  bool              isVar = false;

public:
  explicit Default(FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Default; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif