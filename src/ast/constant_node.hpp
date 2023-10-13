#ifndef QAT_AST_CONSTANT_HPP
#define QAT_AST_CONSTANT_HPP

#include "./node.hpp"

namespace qat::ast {

class PrerunNode : public Node {
public:
  PrerunNode(FileRange _fileRange) : Node(std::move(_fileRange)) {}

  useit bool isPrerunNode() const final { return true; }
  void       createModule(IR::Context* ctx) const final {}
  void       defineType(IR::Context* ctx) final {}
  void       define(IR::Context* ctx) final {}
  useit IR::PrerunValue* emit(IR::Context* ctx) override = 0;
  useit Json             toJson() const override         = 0;
  useit NodeType         nodeType() const override       = 0;
  static void            clearAll();
};

} // namespace qat::ast

#endif