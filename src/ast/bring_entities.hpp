#ifndef QAT_AST_BRING_ENTITIES_HPP
#define QAT_AST_BRING_ENTITIES_HPP

#include "../IR/context.hpp"
#include "../utils/file_range.hpp"
#include "./node.hpp"
#include "./node_type.hpp"

namespace qat::ast {

class BroughtGroup {
  friend class BringEntities;

private:
  u32                relative;
  Vec<Identifier>    entity;
  Vec<BroughtGroup*> members;
  FileRange          fileRange;

  mutable bool             isAlreadyBrought = false;
  mutable ir::EntityState* entityState      = nullptr;

public:
  BroughtGroup(u32 _relative, Vec<Identifier> _entity, Vec<BroughtGroup*> _members, FileRange _fileRange)
      : relative(_relative), entity(_entity), members(std::move(_members)), fileRange(std::move(_fileRange)) {}

  BroughtGroup(u32 _relative, Vec<Identifier> _entity, FileRange _fileRange)
      : relative(_relative), entity(_entity), fileRange(_fileRange) {}

  useit static inline BroughtGroup* create(u32 _relative, Vec<Identifier> _parent, Vec<BroughtGroup*> _members,
                                           FileRange _fileRange) {
    return std::construct_at(OwnNormal(BroughtGroup), _relative, _parent, _members, _fileRange);
  }

  useit static inline BroughtGroup* create(u32 _relative, Vec<Identifier> _parent, FileRange _range) {
    return std::construct_at(OwnNormal(BroughtGroup), _relative, _parent, _range);
  }

  void addMember(BroughtGroup* mem);
  void extendFileRange(FileRange end);
  void bring() const;

  useit bool hasMembers() const;
  useit bool isAllBrought() const;
  useit Json to_json() const;
};

class BringEntities final : public IsEntity {
  Vec<BroughtGroup*>    entities;
  Maybe<VisibilitySpec> visibSpec;

  mutable bool throwErrorsWhenUnfound = false;

public:
  BringEntities(Vec<BroughtGroup*> _entities, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : IsEntity(_fileRange), entities(_entities), visibSpec(_visibSpec) {}

  useit static inline BringEntities* create(Vec<BroughtGroup*> _entities, Maybe<VisibilitySpec> _visibSpec,
                                            FileRange _fileRange) {
    return std::construct_at(OwnNormal(BringEntities), _entities, _visibSpec, _fileRange);
  }

  void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;
  void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;
  void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

  void handle_brings(ir::Mod* mod, ir::Ctx* irCtx) const;

  useit Json     to_json() const final;
  useit NodeType nodeType() const final { return NodeType::BRING_ENTITIES; }
  ~BringEntities() final;
};

} // namespace qat::ast

#endif