#ifndef QAT_AST_CONSTANT_HPP
#define QAT_AST_CONSTANT_HPP

#include "./node.hpp"

namespace qat::ast {

class ConstantNode : public Node {
public:
  ConstantNode(FileRange _fileRange) : Node(std::move(_fileRange)) {}

  useit bool isConstantNode() const final { return true; }
  void       createModule(IR::Context* ctx) const final {}
  void       defineType(IR::Context* ctx) final {}
  void       define(IR::Context* ctx) final {}
  useit IR::ConstantValue* emit(IR::Context* ctx) override = 0;
  useit Json               toJson() const override         = 0;
  useit NodeType           nodeType() const override       = 0;
  static void              clearAll();
};

} // namespace qat::ast

#endif