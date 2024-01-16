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
  DefineRegion(Identifier _name, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : Node(_fileRange), name(_name), visibSpec(_visibSpec) {}

  useit static inline DefineRegion* create(Identifier _name, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineRegion), _name, _visibSpec, _fileRange);
  }

  void  defineType(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::DEFINE_REGION; }
};

} // namespace qat::ast

#endif