#ifndef QAT_AST_EXPRESSION_HPP
#define QAT_AST_EXPRESSION_HPP

#include "./node.hpp"

namespace qat::ast {

// Nature of the expression
//
// "assignable" & "temporary" can be grouped to the generalised nature glvalue
// "pure" and "temporary" can be grouped to the generalsied nature rvalue
//
// Assignable expressions can be assigned to
enum class ExpressionKind {
  assignable, // lvalue
  temporary,  // xvalue
  pure,       // prvalue
};

class Expression : public Node {
private:
  ExpressionKind expected;

protected:
  Maybe<IR::QatType*> inferredType;
  IR::LocalValue*     localValue = nullptr;
  IR::Value*          createIn   = nullptr;

public:
  Expression(FileRange _fileRange);

  useit inline bool isLocalDecl() const { return localValue != nullptr; }
  inline void       setLocalValue(IR::LocalValue* _localValue) { localValue = _localValue; }

  useit inline bool canCreateIn() const { return createIn != nullptr; }
  inline void       setCreateIn(IR::Value* _createIn) { createIn = _createIn; }

  useit inline bool isTypeInferred() const { return inferredType.has_value(); }
  inline void       setInferenceType(IR::QatType* _type) { inferredType = _type; }

  void                 setExpectedKind(ExpressionKind _kind);
  useit ExpressionKind getExpectedKind();
  useit bool           isExpectedKind(ExpressionKind _kind);
  useit IR::Value* emit(IR::Context* ctx) override = 0;
  useit NodeType   nodeType() const override       = 0;
  useit Json       toJson() const override         = 0;
  ~Expression() override                           = default;
};

class PrerunExpression : public Expression {
public:
  PrerunExpression(FileRange fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx) override = 0;
  useit NodeType         nodeType() const override       = 0;
  useit Json             toJson() const override         = 0;
  useit virtual String   toString() const;
  ~PrerunExpression() override = default;
};

} // namespace qat::ast

#endif