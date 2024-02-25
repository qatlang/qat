#ifndef QAT_AST_DO_SKILL_HPP
#define QAT_AST_DO_SKILL_HPP

#include "destructor.hpp"
#include "member_function.hpp"
#include "member_parent_like.hpp"
#include "node.hpp"
#include "operator_function.hpp"
#include "types/qat_type.hpp"
namespace qat::ast {

struct SkillName {
  u32             relative;
  Vec<Identifier> names;
};

class DoSkill final : public IsEntity, public MemberParentLike {
  bool             isDefaultSkill;
  Maybe<SkillName> name;
  ast::QatType*    targetType;

  mutable IR::DoneSkill*    doneSkill = nullptr;
  mutable IR::MemberParent* parent    = nullptr;

public:
  DoSkill(bool _isDef, Maybe<SkillName> _name, ast::QatType* _targetType, FileRange _fileRange)
      : IsEntity(_fileRange), isDefaultSkill(_isDef), name(_name), targetType(_targetType) {}

  useit static inline DoSkill* create(bool _isDef, Maybe<SkillName> _name, ast::QatType* _targetType,
                                      FileRange _fileRange) {
    return std::construct_at(OwnNormal(DoSkill), _isDef, _name, _targetType, _fileRange);
  }

  void create_entity(IR::QatModule* parent, IR::Context* ctx) final;
  void update_entity_dependencies(IR::QatModule* parent, IR::Context* ctx) final;
  void do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) final;

  void define_done_skill(IR::QatModule* mod, IR::Context* ctx);
  void define_members(IR::Context* ctx);
  void emit_members(IR::Context* ctx);

  useit bool     is_done_skill() const final { return true; }
  useit DoSkill* as_done_skill() final { return this; }

  Json     toJson() const final;
  NodeType nodeType() const final { return NodeType::DO_SKILL; }
};

} // namespace qat::ast

#endif