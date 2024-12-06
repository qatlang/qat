#ifndef QAT_AST_PRERUN_ENTITY_HPP
#define QAT_AST_PRERUN_ENTITY_HPP

#include "../expression.hpp"
#include "member_access.hpp"

namespace qat::ast {

class PrerunEntity final : public PrerunExpression {
  friend class PrerunMemberAccess;

private:
  u32             relative;
  Vec<Identifier> identifiers;

public:
  PrerunEntity(u32 _relative, Vec<Identifier> _ids, FileRange _fileRange)
      : PrerunExpression(_fileRange), relative(_relative), identifiers(_ids) {}

  useit static PrerunEntity* create(u32 _relative, Vec<Identifier> _ids, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunEntity), _relative, _ids, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

  useit ir::PrerunValue* emit(EmitCtx* ctx) override;
  useit Json             to_json() const override;
  useit String           to_string() const final;
  useit NodeType         nodeType() const override { return NodeType::PRERUN_ENTITY; }
};

} // namespace qat::ast

#endif
