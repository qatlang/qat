#ifndef QAT_AST_LIB_HPP
#define QAT_AST_LIB_HPP

#include "../IR/context.hpp"
#include "../utils/visibility.hpp"
#include "./node.hpp"
#include <vector>

namespace qat::ast {

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
      utils::VisibilityInfo _visibility, utils::FileRange _file_range);

  IR::Value *emit(IR::Context *ctx);

  nuo::Json toJson();

  NodeType nodeType() const { return NodeType::lib; }

  void destroy();

  ~Lib() noexcept;
};

} // namespace qat::ast

#endif