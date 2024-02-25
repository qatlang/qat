#ifndef QAT_AST_GLOBAL_DECLARATION_HPP
#define QAT_AST_GLOBAL_DECLARATION_HPP

#include "../IR/context.hpp"
#include "./expression.hpp"
#include "./node.hpp"
#include "./node_type.hpp"
#include "./types/qat_type.hpp"
#include "meta_info.hpp"

namespace qat::ast {

class GlobalDeclaration : public IsEntity {
private:
  Identifier            name;
  QatType*              type;
  Maybe<Expression*>    value;
  bool                  isVariable;
  Maybe<VisibilitySpec> visibSpec;
  Maybe<MetaInfo>       metaInfo;

public:
  GlobalDeclaration(Identifier _name, QatType* _type, Maybe<Expression*> _value, bool _isVariable,
                    Maybe<VisibilitySpec> _visibSpec, Maybe<MetaInfo> _metaInfo, FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), type(_type), value(_value), isVariable(_isVariable), visibSpec(_visibSpec),
        metaInfo(_metaInfo) {}

  useit static inline GlobalDeclaration* create(Identifier _name, QatType* _type, Maybe<Expression*> _value,
                                                bool _isVariable, Maybe<VisibilitySpec> _visibSpec,
                                                Maybe<MetaInfo> _metaInfo, FileRange _fileRange) {
    return std::construct_at(OwnNormal(GlobalDeclaration), _name, _type, _value, _isVariable, _visibSpec, _metaInfo,
                             _fileRange);
  }

  void create_entity(IR::QatModule* parent, IR::Context* ctx);
  void update_entity_dependencies(IR::QatModule* parent, IR::Context* ctx);
  void do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx);
  void define(IR::QatModule* mod, IR::Context* ctx);

  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::GLOBAL_DECLARATION; }
};

} // namespace qat::ast

#endif