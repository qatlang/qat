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
  Type*                 type;
  Maybe<Expression*>    value;
  bool                  is_variable;
  Maybe<VisibilitySpec> visibSpec;
  Maybe<MetaInfo>       metaInfo;

public:
  GlobalDeclaration(Identifier _name, Type* _type, Maybe<Expression*> _value, bool _isVariable,
                    Maybe<VisibilitySpec> _visibSpec, Maybe<MetaInfo> _metaInfo, FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), type(_type), value(_value), is_variable(_isVariable), visibSpec(_visibSpec),
        metaInfo(_metaInfo) {}

  useit static GlobalDeclaration* create(Identifier _name, Type* _type, Maybe<Expression*> _value, bool _isVariable,
                                         Maybe<VisibilitySpec> _visibSpec, Maybe<MetaInfo> _metaInfo,
                                         FileRange _fileRange) {
    return std::construct_at(OwnNormal(GlobalDeclaration), _name, _type, _value, _isVariable, _visibSpec, _metaInfo,
                             _fileRange);
  }

  void create_entity(ir::Mod* parent, ir::Ctx* irCtx);
  void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx);
  void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx);
  void define(ir::Mod* mod, ir::Ctx* irCtx);

  useit Json     to_json() const final;
  useit NodeType nodeType() const final { return NodeType::GLOBAL_DECLARATION; }
};

} // namespace qat::ast

#endif
