#ifndef QAT_AST_BOX_HPP
#define QAT_AST_BOX_HPP

#include "../utils/visibility.hpp"
#include "./node.hpp"
#include <optional>
#include <string>
#include <vector>

namespace qat {
namespace AST {
/**
 * Box is a container for objects in the
 * QAT language. It is not a structural container,
 * but exists merely to avoid conflict between
 * libraries
 *
 */
class Box : public Node {
  std::string name;

  std::vector<Node *> members;

  std::optional<utils::VisibilityInfo> visibility;

public:
  Box(std::string _name, std::vector<Node *> _members,
      std::optional<utils::VisibilityInfo> _visibility,
      utils::FilePlacement _filePlacement)
      : name(_name), members(_members), visibility(_visibility),
        Node(_filePlacement) {}

  llvm::Value *emit(IR::Generator *generator);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON() const;

  NodeType nodeType() const { return NodeType::box; }
};
} // namespace AST
} // namespace qat

#endif