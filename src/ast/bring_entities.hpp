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
  String           parent;
  Vec<String>      members;
  utils::FileRange fileRange;

public:
  BroughtGroup(String _parent, Vec<String> _members,
               utils::FileRange _fileRange);
  BroughtGroup(String _parent, utils::FileRange _range);

  useit String getParent() const;
  useit Vec<String> getMembers() const;
  useit bool        isAllBrought() const;
  useit nuo::Json toJson() const;
};

class BringEntities : public Node {
private:
  Vec<BroughtGroup *>   entities;
  utils::VisibilityInfo visibility;

public:
  BringEntities(Vec<BroughtGroup *>          _entities,
                const utils::VisibilityInfo &_visibility,
                utils::FileRange             _fileRange);

  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::bringEntities; }
};

} // namespace qat::ast

#endif