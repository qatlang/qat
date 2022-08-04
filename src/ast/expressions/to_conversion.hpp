#ifndef QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP
#define QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class ToConversion : public Expression {
private:
  Expression *source;
  QatType    *destinationType;

public:
  ToConversion(Expression *_source, QatType *_destinationType,
               utils::FileRange _fileRange)
      : source(_source), destinationType(_destinationType),
        Expression(std::move(_fileRange)) {}

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::toConversion; };
};

} // namespace qat::ast

#endif