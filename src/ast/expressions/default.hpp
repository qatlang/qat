#ifndef QAT_AST_EXPRESSIONS_DEFAULT_HPP
#define QAT_AST_EXPRESSIONS_DEFAULT_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Default : public Expression, public LocalDeclCompatible, public TypeInferrable {
  friend class LocalDeclaration;
  friend class Assignment;

private:
  Maybe<ast::QatType*> providedType;

public:
  Default(Maybe<ast::QatType*> _providedType, FileRange _fileRange);

  LOCAL_DECL_COMPATIBLE_FUNCTIONS
  TYPE_INFERRABLE_FUNCTIONS

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::Default; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif