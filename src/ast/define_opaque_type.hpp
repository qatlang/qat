#ifndef QAT_AST_DEFINE_OPAQUE_TYPE_HPP
#define QAT_AST_DEFINE_OPAQUE_TYPE_HPP

#include "expression.hpp"
#include "meta_info.hpp"
#include "node.hpp"

namespace qat::ast {

class DefineOpaqueType : public Node {
  Identifier               name;
  Maybe<PrerunExpression*> condition;
  Maybe<VisibilitySpec>    visibSpec;
  Maybe<MetaInfo>          metaInfo;

public:
  DefineOpaqueType(Identifier _name, Maybe<PrerunExpression*> _condition, Maybe<VisibilitySpec> _visibSpec,
                   Maybe<MetaInfo> _metaInfo, FileRange _fileRange)
      : Node(_fileRange), name(_name), condition(_condition), visibSpec(_visibSpec), metaInfo(_metaInfo) {}

  useit static inline DefineOpaqueType* create(Identifier _name, Maybe<PrerunExpression*> _condition,
                                               Maybe<VisibilitySpec> _visibSpec, Maybe<MetaInfo> _metaInfo,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(DefineOpaqueType), _name, _condition, _visibSpec, _metaInfo, _fileRange);
  }

  void  define(IR::Context* ctx) final;
  useit IR::Value* emit(IR::Context* ctx) final { return nullptr; }
  useit Json       toJson() const final;
  useit NodeType   nodeType() const final { return NodeType::DEFINE_OPAQUE_TYPE; }
};

} // namespace qat::ast

#endif