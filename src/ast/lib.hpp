#ifndef QAT_AST_LIB_HPP
#define QAT_AST_LIB_HPP

#include "../IR/context.hpp"
#include "../utils/identifier.hpp"
#include "../utils/visibility.hpp"
#include "./node.hpp"
#include <vector>

namespace qat::ast {

// Library in the language
class Lib : public Node {
private:
  Identifier            name;
  Vec<Node*>            members;
  utils::VisibilityKind visibility;

public:
  Lib(Identifier _name, Vec<Node*> _members, utils::VisibilityKind _visibility, const FileRange& _file_range);

  void  createModule(IR::Context* ctx) const final;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::lib; }
};

} // namespace qat::ast

#endif