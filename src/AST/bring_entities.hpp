#ifndef QAT_AST_BRING_ENTITIES_HPP
#define QAT_AST_BRING_ENTITIES_HPP

#include "../IR/context.hpp"
#include "../utils/file_placement.hpp"
#include "./expressions/string_literal.hpp"
#include "./node.hpp"
#include "./node_type.hpp"

namespace qat::AST {

class BroughtGroup {
private:
  /**
   *  The parent entity of the group
   *
   */
  StringLiteral *parent;

  /**
   *  All the members of the parent that should be brought in
   *
   */
  std::vector<StringLiteral *> members;

public:
  BroughtGroup(StringLiteral *_parent, std::vector<StringLiteral *> _members);

  explicit BroughtGroup(StringLiteral *_parent);

  /**
   *  Get the parent entity
   *
   * @return std::string
   */
  StringLiteral *get_parent() const;

  /**
   *  Get the members of the parent entity
   *
   * @return std::vector<std::string>
   */
  std::vector<StringLiteral *> get_members() const;

  /**
   *  Whether the entire entity should be brought into the scope
   *
   * @return true
   * @return false
   */
  bool is_all_brought() const;

  backend::JSON toJSON() const;
};

class BringEntities : public Node {
private:
  /**
   *  All entities to be brought in
   *
   */
  std::vector<BroughtGroup *> entities;

  utils::VisibilityInfo visibility;

public:
  BringEntities(std::vector<BroughtGroup *> _entities,
                utils::VisibilityInfo _visibility,
                utils::FilePlacement _filePlacement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const override;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::bringEntities; }
};

} // namespace qat::AST

#endif