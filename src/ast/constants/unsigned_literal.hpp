#ifndef QAT_AST_CONSTANTS_UNSIGNED_LITERAL_HPP
#define QAT_AST_CONSTANTS_UNSIGNED_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

class UnsignedLiteral : public PrerunExpression {
private:
  String value;

public:
  UnsignedLiteral(String _value, FileRange _fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::unsignedLiteral; }
};

} // namespace qat::ast

#endif