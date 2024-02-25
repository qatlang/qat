#ifndef QAT_AST_BOX_HPP
#define QAT_AST_BOX_HPP

#include "../utils/visibility.hpp"
#include "./node.hpp"

namespace qat::ast {

class Box : public Node {
  Identifier            name;
  Vec<Node*>            members;
  Maybe<VisibilitySpec> visibSpec;

public:
  Box(Identifier _name, Vec<Node*> _members, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : Node(_fileRange), name(_name), members(_members), visibSpec(_visibSpec) {}

  useit static inline Box* create(Identifier _name, Vec<Node*> _members, Maybe<VisibilitySpec> _visibSpec,
                                  FileRange _fileRange) {
    return std::construct_at(OwnNormal(Box), _name, _members, _visibSpec, _fileRange);
  }

  void createModule(IR::Context* ctx) const final;

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::BOX; }
};

} // namespace qat::ast

#endif