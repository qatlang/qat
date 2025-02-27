#ifndef QAT_AST_DEFINE_FLAG_TYPE_HPP
#define QAT_AST_DEFINE_FLAG_TYPE_HPP

#include "./node.hpp"

namespace qat::ast {

struct FlagVariant {
	Vec<Identifier>   names;
	PrerunExpression* value;
	bool              isDefault;
	FileRange         range;

	useit Json to_json() const;
};

class DefineFlagType final : public IsEntity {
	Identifier            name;
	Vec<FlagVariant>      variants;
	Type*                 providedType;
	Maybe<VisibilitySpec> visibSpec;

  public:
	DefineFlagType(Identifier _name, Vec<FlagVariant> _variants, Type* _providedType, Maybe<VisibilitySpec> _visibSpec,
	               FileRange _range)
	    : IsEntity(std::move(_range)), name(std::move(_name)), variants(std::move(_variants)),
	      providedType(_providedType), visibSpec(_visibSpec) {}

	useit static DefineFlagType* create(Identifier name, Vec<FlagVariant> variants, Type* providedType,
	                                    Maybe<VisibilitySpec> visibSpec, FileRange range) {
		return std::construct_at(OwnNormal(DefineFlagType), std::move(name), std::move(variants), providedType,
		                         std::move(visibSpec), std::move(range));
	}

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	useit Json to_json() const final;

	useit NodeType nodeType() const final { return NodeType::DEFINE_FLAG_TYPE; }
};

} // namespace qat::ast

#endif
