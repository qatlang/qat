#ifndef QAT_AST_DEFINE_REGION_HPP
#define QAT_AST_DEFINE_REGION_HPP

#include "node.hpp"
#include "node_type.hpp"
namespace qat::ast {

class DefineRegion : public Node {
private:
  Identifier            name;
  Maybe<VisibilitySpec> visibSpec;

public:
  DefineRegion(Identifier name, Maybe<VisibilitySpec> visibSpec, FileRange fileRange);

  void  defineType(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::defineRegion; }
};

} // namespace qat::ast

#endif