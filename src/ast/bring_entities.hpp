#ifndef QAT_AST_BRING_ENTITIES_HPP
#define QAT_AST_BRING_ENTITIES_HPP

#include "../IR/context.hpp"
#include "../utils/file_range.hpp"
#include "./constants/string_literal.hpp"
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

  ///

  mutable bool isAlreadyBrought = false;

public:
  BroughtGroup(u32 _relative, Vec<Identifier> _parent, Vec<BroughtGroup*> _members, FileRange _fileRange);
  BroughtGroup(u32 _relative, Vec<Identifier> _parent, FileRange _range);

  void addMember(BroughtGroup* mem);
  void extendFileRange(FileRange end);
  void bring() const;

  useit bool hasMembers() const;
  useit bool isAllBrought() const;
  useit Json toJson() const;
};

class BringEntities final : public Node {
private:
  Vec<BroughtGroup*>    entities;
  utils::VisibilityKind visibility;

  mutable bool initialRunComplete = false;

public:
  BringEntities(Vec<BroughtGroup*> _entities, utils::VisibilityKind _visibility, FileRange _fileRange);

  void  handleBrings(IR::Context* ctx) const final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::bringEntities; }
  ~BringEntities() final;
};

} // namespace qat::ast

#endif