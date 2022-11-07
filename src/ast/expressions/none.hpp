#ifndef QAT_AST_NONE_HPP
#define QAT_AST_NONE_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class NoneExpression : public Expression {
  friend class Assignment;
  friend class LocalDeclaration;
  QatType* type = nullptr;

public:
  NoneExpression(QatType* _type, utils::FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::none; }
};

} // namespace qat::ast

#endif