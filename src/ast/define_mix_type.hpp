#ifndef QAT_AST_DEFINE_MIX_TYPE_HPP
#define QAT_AST_DEFINE_MIX_TYPE_HPP

#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "node_type.hpp"

namespace qat::ast {

class DefineMixType final : public IsEntity {
  Vec<Pair<Identifier, Maybe<QatType*>>> subtypes;

  Identifier            name;
  bool                  isPacked;
  Maybe<VisibilitySpec> visibSpec;
  Vec<FileRange>        fRanges;
  Maybe<usize>          defaultVal;

  IR::OpaqueType* opaquedType = nullptr;

public:
  DefineMixType(Identifier _name, Vec<Pair<Identifier, Maybe<QatType*>>> _subTypes, Vec<FileRange> _ranges,
                Maybe<usize> _defaultVal, bool _isPacked, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), subtypes(_subTypes), isPacked(_isPacked), visibSpec(_visibSpec),
        fRanges(_ranges), defaultVal(_defaultVal) {}

  useit static inline DefineMixType* create(Identifier _name, Vec<Pair<Identifier, Maybe<QatType*>>> _subTypes,
                                            Vec<FileRange> _ranges, Maybe<usize> _defaultVal, bool _isPacked,
                                            Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineMixType), _name, _subTypes, _ranges, _defaultVal, _isPacked, _visibSpec,
                             _fileRange);
  }

  void create_opaque(IR::QatModule* mod, IR::Context* ctx);
  void create_type(IR::QatModule* mod, IR::Context* ctx);

  void create_entity(IR::QatModule* parent, IR::Context* ctx) final;
  void update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) final;
  void do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) final;

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::DEFINE_MIX_TYPE; }
};

} // namespace qat::ast

#endif