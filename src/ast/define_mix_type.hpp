#ifndef QAT_AST_DEFINE_MIX_TYPE_HPP
#define QAT_AST_DEFINE_MIX_TYPE_HPP

#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "node_type.hpp"

namespace qat::ast {

class DefineMixType : public Node {
private:
  Identifier                             name;
  Vec<Pair<Identifier, Maybe<QatType*>>> subtypes;
  bool                                   isPacked;
  Vec<QatType*>                          generics;
  utils::VisibilityKind                  visibility;
  Vec<FileRange>                         fRanges;
  Maybe<usize>                           defaultVal;

public:
  DefineMixType(Identifier name, Vec<Pair<Identifier, Maybe<QatType*>>> subTypes, Vec<FileRange> ranges,
                Maybe<usize> defaultVal, bool isPacked, utils::VisibilityKind visibility, FileRange fileRange);

  void       createType(IR::Context* ctx);
  void       defineType(IR::Context* ctx) final;
  void       define(IR::Context* ctx) final {}
  useit bool isGeneric() const;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::defineMixType; }
};

} // namespace qat::ast

#endif