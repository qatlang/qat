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

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(expr);
  }

  useit IR::PrerunValue* emit(IR::Context* ctx);
  useit Json             toJson() const;
  useit String           toString() const;
  useit NodeType         nodeType() const { return NodeType::PRERUN_MEMBER_ACCESS; }
};

} // namespace qat::ast

#endif