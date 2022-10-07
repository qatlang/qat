#ifndef QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP
#define QAT_EXPRESSIONS_CONSTRUCTOR_CALL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ConstructorCall : public Expression {
  friend class LocalDeclaration;

private:
  QatType*         type;
  Vec<Expression*> args;
  bool             isHeaped;

  mutable IR::LocalValue* local = nullptr;
  mutable String          irName;
  mutable bool            isVar = true;

public:
  ConstructorCall(QatType* _type, Vec<Expression*> _args, bool _isHeap, utils::FileRange _fileRange);

  useit IR::Value* emit(IR::Context* ctx) final;
  useit NodeType   nodeType() const final { return NodeType::constructorCall; }
  useit Json       toJson() const final;
};

} // namespace qat::ast

#endif