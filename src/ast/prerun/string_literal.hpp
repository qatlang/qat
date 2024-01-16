#ifndef QAT_AST_CONSTANTS_STRING_LITERAL_HPP
#define QAT_AST_CONSTANTS_STRING_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"
#include "../node_type.hpp"

namespace qat::ast {

class StringLiteral : public PrerunExpression {
  String value;

public:
  StringLiteral(String _value, FileRange _fileRange)
      : PrerunExpression(std::move(_fileRange)), value(std::move(_value)) {}

  useit static inline StringLiteral* create(String _value, FileRange _fileRange) {
    return std::construct_at(OwnNormal(StringLiteral), _value, _fileRange);
  }

  void addValue(const String& val, const FileRange& fRange);

  useit String get_value() const;
  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::STRING_LITERAL; }
};

} // namespace qat::ast

#endif