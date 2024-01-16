#ifndef QAT_AST_BRING_BITWIDTHS_HPP
#define QAT_AST_BRING_BITWIDTHS_HPP

#include "node.hpp"
#include "types/qat_type.hpp"

namespace qat::ast {

class BringBitwidths : public Node {
  Vec<ast::QatType*> broughtTypes;

public:
  BringBitwidths(Vec<ast::QatType*> _broughtTypes, FileRange _fileRange)
      : Node(_fileRange), broughtTypes(_broughtTypes) {}

  useit static inline BringBitwidths* create(Vec<ast::QatType*> _broughtTypes, FileRange _fileRange) {
    return std::construct_at(OwnNormal(BringBitwidths), _broughtTypes, _fileRange);
  }

  void           handleBrings(IR::Context* ctx) const final;
  IR::Value*     emit(IR::Context* ctx) final { return nullptr; }
  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::BRING_BITWIDTHS; }
};

} // namespace qat::ast

#endif