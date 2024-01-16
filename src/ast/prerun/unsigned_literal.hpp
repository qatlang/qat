#ifndef QAT_AST_CONSTANTS_UNSIGNED_LITERAL_HPP
#define QAT_AST_CONSTANTS_UNSIGNED_LITERAL_HPP

#include "../../IR/context.hpp"
#include "../expression.hpp"

namespace qat::ast {

class UnsignedLiteral : public PrerunExpression, public TypeInferrable {
private:
  String                      value;
  Maybe<Pair<u64, FileRange>> bits;

public:
  UnsignedLiteral(String _value, Maybe<Pair<u64, FileRange>> _bits, FileRange _fileRange)
      : PrerunExpression(_fileRange), value(_value), bits(_bits) {}

  useit static inline UnsignedLiteral* create(String _value, Maybe<Pair<u64, FileRange>> bits, FileRange _fileRange) {
    return std::construct_at(OwnNormal(UnsignedLiteral), _value, bits, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::UNSIGNED_LITERAL; }
};

} // namespace qat::ast

#endif