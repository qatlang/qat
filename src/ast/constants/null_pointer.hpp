#ifndef QAT_AST_CONSTANTS_NULL_POINTER_HPP
#define QAT_AST_CONSTANTS_NULL_POINTER_HPP

#include "../expression.hpp"

namespace qat::ast {

/**
 *  A null pointer
 *
 */
class NullPointer : public PrerunExpression, public TypeInferrable {
public:
  explicit NullPointer(FileRange _fileRange);

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::nullPointer; }
};

} // namespace qat::ast

#endif