#ifndef QAT_AST_PRERUN_NULL_POINTER_HPP
#define QAT_AST_PRERUN_NULL_POINTER_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

/**
 *  A null pointer
 *
 */
class NullPointer final : public PrerunExpression, public TypeInferrable {
  Maybe<ast::Type*> providedType;

public:
  NullPointer(Maybe<ast::Type*> _providedType, FileRange _fileRange)
      : PrerunExpression(_fileRange), providedType(_providedType) {}

  useit static inline NullPointer* create(Maybe<ast::Type*> _providedType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(NullPointer), _providedType, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    if (providedType.has_value()) {
      UPDATE_DEPS(providedType.value());
    }
  }

  TYPE_INFERRABLE_FUNCTIONS

  useit ir::PrerunValue* emit(EmitCtx* ctx) override;
  useit Json             to_json() const override;
  useit String           to_string() const final;
  useit NodeType         nodeType() const override { return NodeType::NULL_POINTER; }
};

} // namespace qat::ast

#endif