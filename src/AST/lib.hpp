#ifndef QAT_AST_LIB_HPP
#define QAT_AST_LIB_HPP

#include "../IR/context.hpp"
#include "../utils/visibility.hpp"
#include "./node.hpp"
#include <vector>

namespace qat {
namespace AST {

// Library in the language
class Lib : public Node {
private:
  // Name of the library
  std::string name;

  // All node members of this library
  std::vector<Node *> members;

  // Visibility of this library
  utils::VisibilityInfo visibility;

public:
  Lib(std::string _name, std::vector<Node *> _members,
      utils::VisibilityInfo _visibility, utils::FilePlacement _file_placement);

  IR::Value *emit(IR::Context *ctx);

  void emitCPP(backend::cpp::File &file, bool isHeader) const;

  backend::JSON toJSON();

  NodeType nodeType() const { return NodeType::lib; }
};
} // namespace AST
} // namespace qat

#endif