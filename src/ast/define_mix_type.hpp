#ifndef QAT_AST_DEFINE_MIX_TYPE_HPP
#define QAT_AST_DEFINE_MIX_TYPE_HPP

#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "node_type.hpp"

namespace qat::ast {

class DefineMixType : public Node {
  Identifier                             name;
  Vec<Pair<Identifier, Maybe<QatType*>>> subtypes;
  bool                                   isPacked;
  Vec<QatType*>                          generics;
  Maybe<VisibilitySpec>                  visibSpec;
  Vec<FileRange>                         fRanges;
  Maybe<usize>                           defaultVal;

  IR::OpaqueType* opaquedType = nullptr;

public:
  DefineMixType(Identifier _name, Vec<Pair<Identifier, Maybe<QatType*>>> _subTypes, Vec<FileRange> _ranges,
                Maybe<usize> _defaultVal, bool _isPacked, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : Node(_fileRange), name(_name), subtypes(_subTypes), isPacked(_isPacked), visibSpec(_visibSpec),
        fRanges(_ranges), defaultVal(_defaultVal) {}

  useit static inline DefineMixType* create(Identifier _name, Vec<Pair<Identifier, Maybe<QatType*>>> _subTypes,
                                            Vec<FileRange> _ranges, Maybe<usize> _defaultVal, bool _isPacked,
                                            Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineMixType), _name, _subTypes, _ranges, _defaultVal, _isPacked, _visibSpec,
                             _fileRange);
  }

  void       createType(IR::Context* ctx);
  void       defineType(IR::Context* ctx) final;
  void       define(IR::Context* ctx) final {}
  useit bool isGeneric() const;
  useit IR::Value* emit(IR::Context* ctx) final;
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::DEFINE_MIX_TYPE; }
};

} // namespace qat::ast

#endif