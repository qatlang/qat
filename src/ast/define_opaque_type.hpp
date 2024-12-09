#ifndef QAT_AST_DEFINE_OPAQUE_TYPE_HPP
#define QAT_AST_DEFINE_OPAQUE_TYPE_HPP

#include "expression.hpp"
#include "meta_info.hpp"
#include "node.hpp"

namespace qat::ast {

class DefineOpaqueType final : public IsEntity {
	friend class ir::Mod;
	Identifier               name;
	Maybe<PrerunExpression*> condition;
	Maybe<VisibilitySpec>    visibSpec;
	Maybe<MetaInfo>          metaInfo;

  public:
	DefineOpaqueType(Identifier _name, Maybe<PrerunExpression*> _condition, Maybe<VisibilitySpec> _visibSpec,
	                 Maybe<MetaInfo> _metaInfo, FileRange _fileRange)
	    : IsEntity(_fileRange), name(_name), condition(_condition), visibSpec(_visibSpec), metaInfo(_metaInfo) {}

	useit static DefineOpaqueType* create(Identifier _name, Maybe<PrerunExpression*> _condition,
	                                      Maybe<VisibilitySpec> _visibSpec, Maybe<MetaInfo> _metaInfo,
	                                      FileRange _fileRange) {
		return std::construct_at(OwnNormal(DefineOpaqueType), _name, _condition, _visibSpec, _metaInfo, _fileRange);
	}

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
	void update_entity_dependencies(ir::Mod*, ir::Ctx* irCtx) final;
	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	useit Json     to_json() const final;
	useit NodeType nodeType() const final { return NodeType::DEFINE_OPAQUE_TYPE; }
};

} // namespace qat::ast

#endif
