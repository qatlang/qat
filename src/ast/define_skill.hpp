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

	useit Json to_json() const {
		return Json()
		    ._("hasVisibility", visibSpec.has_value())
		    ._("visibility", visibSpec.has_value() ? visibSpec.value().to_json() : JsonValue())
		    ._("name", name)
		    ._("hasDefineCondition", defineChecker != nullptr)
		    ._("defineCondition", defineChecker ? defineChecker->to_json() : JsonValue())
		    ._("type", type->to_json())
		    ._("fileRange", range);
	}
};

enum class SkillMethodKind {
	STATIC,
	NORMAL,
	VARIATION,
};

useit inline ir::SkillMethodKind method_kind_to_ir(SkillMethodKind kind) {
	switch (kind) {
		case SkillMethodKind::NORMAL:
			return ir::SkillMethodKind::NORMAL;
		case SkillMethodKind::VARIATION:
			return ir::SkillMethodKind::VARIATION;
		case SkillMethodKind::STATIC:
			return ir::SkillMethodKind::STATIC;
	}
}

useit inline String method_kind_to_string(SkillMethodKind kind) {
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

	useit Json to_json() const {
		Vec<JsonValue> argsJSON;
		for (auto& arg : arguments) {
			argsJSON.push_back(arg->to_json());
		}
		return Json()
		    ._("hasVisibility", visibSpec.has_value())
		    ._("visibility", visibSpec.has_value() ? visibSpec.value().to_json() : JsonValue())
		    ._("kind", method_kind_to_string(kind))
		    ._("name", name)
		    ._("arguments", argsJSON)
		    ._("variadics", variadics.has_value() ? variadics.value().to_json() : JsonValue())
		    ._("hasGivenType", givenType != nullptr)
		    ._("givenType", givenType ? givenType->to_json() : JsonValue())
		    ._("hasDefineCondition", defineChecker != nullptr)
		    ._("defineCondition", defineChecker ? defineChecker->to_json() : JsonValue())
		    ._("fileRange", fileRange);
	}
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

	useit static DefineSkill* create(Identifier name, Vec<GenericAbstractType*> generics,
	                                 Maybe<VisibilitySpec> visibSpec, PrerunExpression* polyQualifier,
	                                 Vec<SkillTypeDefinition> typeDefs, Vec<SkillMethod> methods,
	                                 PrerunExpression* defineChecker, PrerunExpression* genericConstraint,
	                                 FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(DefineSkill), std::move(name), std::move(generics), visibSpec, polyQualifier,
		                         std::move(typeDefs), std::move(methods), defineChecker, genericConstraint,
		                         std::move(fileRange));
	}

	useit bool is_generic() const { return not generics.empty(); }

	void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

	useit ir::Skill* create_skill(Vec<ir::GenericToFill*> const& toFill, ir::Mod* parent, ir::Ctx* irCtx);

	void create_type_definitions(ir::Skill* skill, ir::Mod* parent, ir::Ctx* irCtx);

	void create_methods(ir::Skill* skill, ir::Mod* parent, ir::Ctx* irCtx);

	useit NodeType nodeType() const final { return NodeType::DEFINE_SKILL; }

	useit Json to_json() const final {
		Vec<JsonValue> typeJSON;
		for (auto& ty : typeDefinitions) {
			typeJSON.push_back(ty.to_json());
		}
		Vec<JsonValue> methodJSON;
		for (auto& mt : methods) {
			typeJSON.push_back(mt.to_json());
		}
		return Json()._("typeDefinitions", typeJSON)._("methods", methodJSON);
	}
};

} // namespace qat::ast

#endif
