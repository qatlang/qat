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
  StringLiteral *parent;

  // All the members of the parent that should be brought in
  std::vector<StringLiteral *> members;

public:
  BroughtGroup(StringLiteral *_parent, std::vector<StringLiteral *> _members);

  explicit BroughtGroup(StringLiteral *_parent);

  // Get the parent entity
  StringLiteral *get_parent() const;

  // Get the members of the parent entity
  std::vector<StringLiteral *> get_members() const;

  // Whether the entire entity should be brought into the scope
  bool is_all_brought() const;

  nuo::Json toJson() const;
};

class BringEntities : public Node {
private:
  // All entities to be brought in
  std::vector<BroughtGroup *> entities;

  // Visibility of the brought entities
  utils::VisibilityInfo visibility;

public:
  BringEntities(std::vector<BroughtGroup *> _entities,
                utils::VisibilityInfo _visibility, utils::FileRange _fileRange);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const override;

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::bringEntities; }
};

} // namespace qat::ast

#endif