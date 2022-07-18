#ifndef QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP
#define QAT_AST_EXPRESSIONS_TO_CONVERSION_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::AST {

class ToConversion : public Expression {
private:
  Expression *source;
  QatType *destinationType;

public:
  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType();
};

} // namespace qat::AST

#endif