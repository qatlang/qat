#ifndef QAT_AST_NODE_HPP
#define QAT_AST_NODE_HPP

#include "../IR/context.hpp"
#include "../IR/types/array.hpp"
#include "../IR/types/c_type.hpp"
#include "../IR/types/choice.hpp"
#include "../IR/types/core_type.hpp"
#include "../IR/types/float.hpp"
#include "../IR/types/function.hpp"
#include "../IR/types/integer.hpp"
#include "../IR/types/mix.hpp"
#include "../IR/types/pointer.hpp"
#include "../IR/types/reference.hpp"
#include "../IR/types/string_slice.hpp"
#include "../IR/types/tuple.hpp"
#include "../IR/types/unsigned.hpp"
#include "../memory_tracker.hpp"
#include "../show.hpp"
#include "../utils/file_range.hpp"
#include "../utils/helpers.hpp"
#include "../utils/json.hpp"
#include "./node_type.hpp"

namespace qat::ast {

//  Node is the base class for all AST members of the language, and it
// requires a FileRange instance that indicates its position in the
// corresponding file
class Node {
private:
  static Vec<Node*> allNodes;

public:
  FileRange fileRange;

  explicit Node(FileRange _fileRange);
  virtual ~Node() = default;
  useit virtual bool       isConstantNode() const { return false; }
  virtual void             createModule(IR::Context* ctx) const {}
  virtual void             handleFilesystemBrings(IR::Context* ctx) const {}
  virtual void             handleBrings(IR::Context* ctx) const {}
  virtual void             defineType(IR::Context* ctx) {}
  virtual void             define(IR::Context* ctx) {}
  useit virtual IR::Value* emit(IR::Context* ctx) = 0;
  useit virtual Json       toJson() const         = 0;
  useit virtual NodeType   nodeType() const       = 0;
  static void              clearAll();
};

class HolderNode : public Node {
private:
  Node* node;

public:
  explicit HolderNode(Node* _node) : Node(_node->fileRange), node(_node) {}

  // NOLINTNEXTLINE(misc-unused-parameters)
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit Json       toJson() const final { return node->toJson(); }
  useit NodeType   nodeType() const final { return NodeType::holder; }
};

} // namespace qat::ast

#endif
