#ifndef QAT_AST_BRING_ENTITIES_HPP
#define QAT_AST_BRING_ENTITIES_HPP

#include "../IR/context.hpp"
#include "../utils/file_range.hpp"
#include "./expressions/string_literal.hpp"
#include "./node.hpp"
#include "./node_type.hpp"

namespace qat::ast {

class BroughtGroup {
private:
  // The parent entity of the group
  String parent;

  // All the members of the parent that should be brought in
  Vec<String> members;

  // FileRange spanned by this group
  utils::FileRange fileRange;

public:
  BroughtGroup(String _parent, Vec<String> _members,
               utils::FileRange _fileRange);

  BroughtGroup(String _parent, utils::FileRange _range);

  // Get the parent entity
  useit String getParent() const;

  // Get the members of the parent entity
  useit Vec<String> getMembers() const;

  // Whether the entire entity should be brought into the scope
  useit bool is_all_brought() const;

  useit nuo::Json toJson() const;
};

class BringEntities : public Node {
private:
  // All entities to be brought in
  Vec<BroughtGroup *> entities;

  // Visibility of the brought entities
  utils::VisibilityInfo visibility;

public:
  BringEntities(Vec<BroughtGroup *>          _entities,
                const utils::VisibilityInfo &_visibility,
                utils::FileRange             _fileRange);

  IR::Value *emit(IR::Context *ctx) override;

  useit nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::bringEntities; }
};

} // namespace qat::ast

#endif