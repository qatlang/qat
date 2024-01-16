#ifndef QAT_AST_CAST_HPP
#define QAT_AST_CAST_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class Cast : public Expression {
  Expression* instance;
  QatType*    destination;

public:
  Cast(Expression* _mainExp, QatType* _dest, FileRange _fileRange)
      : Expression(_fileRange), instance(_mainExp), destination(_dest) {}

  useit static inline Cast* create(Expression* mainExp, QatType* value, FileRange fileRange) {
    return std::construct_at(OwnNormal(Cast), mainExp, value, fileRange);
  }

  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::CAST; }
};

} // namespace qat::ast

#endif