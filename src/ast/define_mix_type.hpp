#ifndef QAT_AST_DEFINE_MIX_TYPE_HPP
#define QAT_AST_DEFINE_MIX_TYPE_HPP

#include "./node.hpp"
#include "./types/qat_type.hpp"
#include "node_type.hpp"

namespace qat::ast {

class DefineMixType final : public IsEntity {
  Vec<Pair<Identifier, Maybe<Type*>>> subtypes;

  Identifier            name;
  bool                  isPacked;
  Maybe<VisibilitySpec> visibSpec;
  Vec<FileRange>        fRanges;
  Maybe<usize>          defaultVal;

  ir::OpaqueType* opaquedType = nullptr;

public:
  DefineMixType(Identifier _name, Vec<Pair<Identifier, Maybe<Type*>>> _subTypes, Vec<FileRange> _ranges,
                Maybe<usize> _defaultVal, bool _isPacked, Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), subtypes(_subTypes), isPacked(_isPacked), visibSpec(_visibSpec),
        fRanges(_ranges), defaultVal(_defaultVal) {}

  useit static inline DefineMixType* create(Identifier _name, Vec<Pair<Identifier, Maybe<Type*>>> _subTypes,
                                            Vec<FileRange> _ranges, Maybe<usize> _defaultVal, bool _isPacked,
                                            Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineMixType), _name, _subTypes, _ranges, _defaultVal, _isPacked, _visibSpec,
                             _fileRange);
  }

  void create_opaque(ir::Mod* mod, ir::Ctx* irCtx);
  void create_type(ir::Mod* mod, ir::Ctx* irCtx);

  void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
  void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;
  void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

  useit Json     to_json() const final;
  useit NodeType nodeType() const final { return NodeType::DEFINE_MIX_TYPE; }
};

} // namespace qat::ast

#endif