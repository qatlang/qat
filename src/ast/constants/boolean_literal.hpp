#ifndef QAT_AST_CONSTANTS_BOOLEAN_LITERAL_HPP
#define QAT_AST_CONSTANTS_BOOLEAN_LITERAL_HPP

#include "../expression.hpp"

namespace qat::ast {

class BooleanLiteral : public PrerunExpression {
private:
  bool value;

public:
  BooleanLiteral(bool _value, FileRange _fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit String           toString() const final;
  useit NodeType         nodeType() const final { return NodeType::booleanLiteral; }
};

} // namespace qat::ast

#endif