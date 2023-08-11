#ifndef QAT_AST_BOX_HPP
#define QAT_AST_BOX_HPP

#include "../utils/visibility.hpp"
#include "./node.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat::ast {

// Box is a container for objects in the
// QAT language. It is not a structural container,
// but exists merely to avoid conflict between
// libraries
class Box : public Node {
  Identifier     name;
  Vec<Node*>     members;
  VisibilityKind visibility;

public:
  Box(Identifier _name, Vec<Node*> _members, VisibilityKind _visibility, FileRange _fileRange);

  void  createModule(IR::Context* ctx) const final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::box; }
};

} // namespace qat::ast

#endif