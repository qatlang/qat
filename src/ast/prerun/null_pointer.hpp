#ifndef QAT_AST_CONSTANTS_NULL_POINTER_HPP
#define QAT_AST_CONSTANTS_NULL_POINTER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

/**
 *  A null pointer
 *
 */
class NullPointer : public PrerunExpression, public TypeInferrable {
  Maybe<ast::QatType*> providedType;

public:
  NullPointer(Maybe<ast::QatType*> _providedType, FileRange _fileRange)
      : PrerunExpression(_fileRange), providedType(_providedType) {}

  useit static inline NullPointer* create(Maybe<ast::QatType*> _providedType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(NullPointer), _providedType, _fileRange);
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::NULL_POINTER; }
};

} // namespace qat::ast

#endif