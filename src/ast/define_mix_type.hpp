#ifndef QAT_AST_DEFINE_UNION_TYPE_HPP
#define QAT_AST_DEFINT_UNION_TYPE_HPP

#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "node_type.hpp"

namespace qat::ast {

class DefineMixType : public Node {
private:
  String                              name;
  Vec<Pair<String, Maybe<QatType *>>> subtypes;
  bool                                isPacked;
  Vec<QatType *>                      templates;
  utils::VisibilityKind               visibility;
  Vec<utils::FileRange>               fRanges;

public:
  DefineMixType(String name, Vec<Pair<String, Maybe<QatType *>>> subTypes,
                Vec<utils::FileRange> ranges, bool isPacked,
                utils::VisibilityKind visibility, utils::FileRange fileRange);

  void       createType(IR::Context *ctx);
  void       defineType(IR::Context *ctx) final;
  void       define(IR::Context *ctx) final {}
  useit bool isTemplate() const;
  useit IR::Value *emit(IR::Context *ctx) final;
  useit nuo::Json toJson() const final;
  useit NodeType  nodeType() const final { return NodeType::defineMixType; }
};

} // namespace qat::ast

#endif