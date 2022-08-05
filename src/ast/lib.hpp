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
  String name;

  // All node members of this library
  Vec<Node *> members;

  // Visibility of this library
  utils::VisibilityInfo visibility;

public:
  Lib(String _name, Vec<Node *> _members,
      const utils::VisibilityInfo &_visibility, utils::FileRange _file_range);

  IR::Value *emit(IR::Context *ctx) override;

  nuo::Json toJson() const override;

  useit NodeType nodeType() const override { return NodeType::lib; }

  void destroy() override;
};

} // namespace qat::ast

#endif