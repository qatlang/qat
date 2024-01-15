#ifndef QAT_AST_CAST_HPP
#define QAT_AST_CAST_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Cast : public Expression {
  Expression* instance;
  QatType*    destination;

  Cast(Expression* _mainExp, QatType* _value, FileRange _fileRange);

public:
  useit static Cast* Create(Expression* mainExp, QatType* value, FileRange fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::cast; }
};

} // namespace qat::ast

#endif