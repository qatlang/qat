#ifndef QAT_AST_EXPRESSIONS_DEFAULT_HPP
#define QAT_AST_EXPRESSIONS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Default : public Expression {
  friend class LocalDeclaration;

private:
  Maybe<ast::QatType*> providedType;
  Maybe<Identifier>    irName;
  bool                 isVar = false;

public:
  Default(Maybe<ast::QatType*> _providedType, FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Default; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif