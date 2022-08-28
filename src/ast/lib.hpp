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
  String                name;
  Vec<Node *>           members;
  utils::VisibilityKind visibility;

public:
  Lib(const String &_name, Vec<Node *> _members,
      utils::VisibilityKind _visibility, const utils::FileRange &_file_range);

  useit IR::Value *emit(IR::Context *ctx) override;
  useit nuo::Json toJson() const override;
  useit NodeType  nodeType() const override { return NodeType::lib; }
};

} // namespace qat::ast

#endif