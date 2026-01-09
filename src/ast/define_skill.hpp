#ifndef QAT_AST_DEFINE_SKILL_HPP
#define QAT_AST_DEFINE_SKILL_HPP

#include "./argument.hpp"
#include "./expression.hpp"
#include "./node.hpp"
#include "./types/variadics.hpp"

namespace qat::ast {

struct SkillTypeDefinition {
	Maybe<VisibilitySpec> visibSpec;
	Identifier            name;
	PrerunExpression*     defineChecker;
	Type*                 type;
	FileRangePtr          range;

	void update_dependencies(ir::EmitPhase phase, ir::DependType depend, ir::EntityState* ent, EmitCtx* ctx);
};

enum class SkillMethodKind {
	STATIC,
	NORMAL,
	VARIATION,
};

inline ir::SkillMethodKind method_kind_to_ir(SkillMethodKind kind) {
	switch (kind) {
		case SkillMethodKind::NORMAL:
			return ir::SkillMethodKind::NORMAL;
		case SkillMethodKind::VARIATION:
			return ir::SkillMethodKind::VARIATION;
		case SkillMethodKind::STATIC:
			return ir::SkillMethodKind::STATIC;
	}
}

inline String method_kind_to_string(SkillMethodKind kind) {
	switch (kind) {
		case SkillMethodKind::NORMAL:
			return "normal";
		case SkillMethodKind::VARIATION:
			return "variation";
		case SkillMethodKind::STATIC:
			return "static";
	}
}

struct SkillMethod {
	Maybe<VisibilitySpec> visibSpec;
	SkillMethodKind       kind;
	Identifier            name;
	Vec<Argument*>        arguments;
	Maybe<Variadics>      variadics;
	Type*                 givenType     = nullptr;
	PrerunExpression*     defineChecker = nullptr;
	FileRangePtr          fileRange;

	void update_dependencies(ir::EmitPhase phase, ir::DependType depend, ir::EntityState* ent, EmitCtx* ctx);
};

class DefineSkill final : public IsEntity {
	Maybe<VisibilitySpec>     visibSpec;
	Identifier                name;
	PrerunExpression*         polyQualifier;
	Vec<SkillTypeDefinition>  typeDefinitions;
	Vec<SkillMethod>          methods;
	Vec<GenericAbstractType*> generics;
	PrerunExpression*         defineChecker;
	PrerunExpression*         genericConstraint;

	mutable ir::GenericSkill* genericSkill = nullptr;
	mutable ir::Skill*        resultSkill  = nullptr;

  public:
	DefineSkill(Identifier _name, Vec<GenericAbstractType*> _generics, Maybe<VisibilitySpec> _visibSpec,
	            PrerunExpression* _polyQualifier, Vec<SkillTypeDefinition> _typeDefs, Vec<SkillMethod> _methods,
	            PrerunExpression* _defineChecker, PrerunExpression* _genericConstraint, FileRangePtr _fileRange)
	    : IsEntity(std::move(_fileRange)), visibSpec(_visibSpec), name(std::move(_name)), polyQualifier(_polyQualifier),
	      typeDefinitions(std::move(_typeDefs)), methods(std::move(_methods)), generics(std::move(_generics)),
	      defineChecker(_defineChecker), genericConstraint(_genericConstraint) {}

	static DefineSkill* create(Identifier name, Vec<GenericAbstractType*> generics, Maybe<VisibilitySpec> visibSpec,
	                           PrerunExpression* polyQualifier, Vec<SkillTypeDefinition> typeDefs,
	                           Vec<SkillMethod> methods, PrerunExpression* defineChecker,
	                           PrerunExpression* genericConstraint, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(DefineSkill), std::move(name), std::move(generics), visibSpec, polyQualifier,
		                         std::move(typeDefs), std::move(methods), defineChecker, genericConstraint,
		                         std::move(fileRange));
	}

	bool is_generic() const { return not generics.empty(); }

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	ir::Skill* create_skill(Vec<ir::GenericToFill*> const& toFill, ir::Mod* parent, ir::Ctx* irCtx);

	void create_type_definitions(ir::Skill* skill, ir::Mod* parent, ir::Ctx* irCtx);

	void create_methods(ir::Skill* skill, ir::Mod* parent, ir::Ctx* irCtx);

	NodeType nodeType() const final { return NodeType::DEFINE_SKILL; }
};

} // namespace qat::ast

#endif
