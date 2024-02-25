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
  Maybe<VisibilitySpec> visibSpec;

public:
  Lib(Identifier _name, Vec<Node*> _members, Maybe<VisibilitySpec> _visibSpec, const FileRange& _file_range)
      : Node(_file_range), name(_name), members(_members), visibSpec(_visibSpec) {}

  useit static inline Lib* create(Identifier _name, Vec<Node*> _members, Maybe<VisibilitySpec> _visibSpec,
                                  const FileRange& _file_range) {
    return std::construct_at(OwnNormal(Lib), _name, _members, _visibSpec, _file_range);
  }

  void createModule(IR::Context* ctx) const final;

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::LIB; }
};

} // namespace qat::ast

#endif