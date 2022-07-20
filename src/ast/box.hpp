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
  std::string name;

  std::vector<Node *> members;

  std::optional<utils::VisibilityInfo> visibility;

public:
  Box(std::string _name, std::vector<Node *> _members,
      std::optional<utils::VisibilityInfo> _visibility,
      utils::FileRange _fileRange)
      : name(_name), members(_members), visibility(_visibility),
        Node(_fileRange) {}

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson() const;

  NodeType nodeType() const { return NodeType::box; }
};

} // namespace qat::ast

#endif