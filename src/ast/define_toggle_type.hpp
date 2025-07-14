#ifndef QAT_AST_DEFINE_TOGGLE_TYPE_HPP
#define QAT_AST_DEFINE_TOGGLE_TYPE_HPP

#include "../IR/meta_info.hpp"
#include "./expression.hpp"
#include "./meta_info.hpp"
#include "./node.hpp"
#include "./types/generic_abstract.hpp"

namespace qat::ir {
class GenericToggleType;
}

namespace qat::ast {

class DefineToggleType : public IsEntity {
	Identifier                        name;
	Vec<GenericAbstractType*>         generics;
	Vec<Pair<Vec<Identifier>, Type*>> variants;
	PrerunExpression*                 defineChecker;
	PrerunExpression*                 genericConstraint;
	Maybe<MetaInfo>                   metaInfo;
	Maybe<VisibilitySpec>             visibSpec;

	Maybe<bool>          checkResult;
	Vec<ir::OpaqueType*> opaquedTypes;
	Maybe<ir::MetaInfo>  metaIR;

	ir::GenericToggleType* genericToggleType = nullptr;

  public:
	DefineToggleType(Identifier _name, Vec<Pair<Vec<Identifier>, Type*>> _variants, PrerunExpression* _defineChecker,
	                 PrerunExpression* _genericConstraint, Maybe<MetaInfo> _metaInfo, Maybe<VisibilitySpec> _visibSpec,
	                 FileRange _fileRange)
	    : IsEntity(std::move(_fileRange)), name(std::move(_name)), variants(std::move(_variants)),
	      defineChecker(_defineChecker), genericConstraint(_genericConstraint), metaInfo(std::move(_metaInfo)),
	      visibSpec(std::move(_visibSpec)) {}

	useit static DefineToggleType* create(Identifier name, Vec<Pair<Vec<Identifier>, Type*>> variants,
	                                      PrerunExpression* defineChecker, PrerunExpression* genericConstraint,
	                                      Maybe<MetaInfo> metaInfo, Maybe<VisibilitySpec> visibSpec,
	                                      FileRange fileRange) {
		return std::construct_at(OwnNormal(DefineToggleType), std::move(name), std::move(variants), defineChecker,
		                         genericConstraint, std::move(metaInfo), std::move(visibSpec), std::move(fileRange));
	}

	useit bool is_generic() const { return not generics.empty(); }

	void create_opaque(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* mod, ir::Ctx* irCtx);

	void set_opaque(ir::OpaqueType* opaque) { opaquedTypes.push_back(opaque); }

	ir::OpaqueType* get_opaque() const { return opaquedTypes.back(); }

	void unset_opaque() { opaquedTypes.pop_back(); }

	useit ir::ToggleType* create_type(Vec<ir::GenericToFill*> const& genericsToFill, ir::Mod* mod, ir::Ctx* irCtx);

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::DEFINE_TOGGLE_TYPE; }
};

} // namespace qat::ast

#endif
