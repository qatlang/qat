#ifndef QAT_AST_PRERUN_MEMBER_ACCESS_HPP
#define QAT_AST_PRERUN_MEMBER_ACCESS_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunMemberAccess final : public PrerunExpression {
  PrerunExpression* expr;
  Identifier        memberName;

public:
  PrerunMemberAccess(PrerunExpression* _expr, Identifier _member, FileRange _fileRange)
      : PrerunExpression(std::move(_fileRange)), expr(_expr), memberName(_member) {}

  useit static inline PrerunMemberAccess* create(PrerunExpression* _expr, Identifier _member, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunMemberAccess), _expr, _member, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
    UPDATE_DEPS(expr);
  }

  useit ir::PrerunValue* emit(EmitCtx* ctx);
  useit Json             to_json() const;
  useit String           to_string() const;
  useit NodeType         nodeType() const { return NodeType::PRERUN_MEMBER_ACCESS; }
};

} // namespace qat::ast

#endif