#ifndef QAT_AST_CONSTANTS_ENTITY_HPP
#define QAT_AST_CONSTANTS_ENTITY_HPP

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

  useit static inline PrerunEntity* create(u32 _relative, Vec<Identifier> _ids, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunEntity), _relative, _ids, _fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final;

  useit IR::PrerunValue* emit(IR::Context* ctx) override;
  useit Json             toJson() const override;
  useit String           toString() const final;
  useit NodeType         nodeType() const override { return NodeType::PRERUN_ENTITY; }
};

} // namespace qat::ast

#endif