#ifndef QAT_AST_DEFINE_PRERUN_FUNCTION_HPP
#define QAT_AST_DEFINE_PRERUN_FUNCTION_HPP

#include "./argument.hpp"
#include "./node.hpp"

namespace qat::ast {

class DefinePrerunFunction : public IsEntity {
	Identifier            name;
	Type*                 returnType;
	Vec<Argument*>        arguments;
	Vec<PrerunSentence*>  sentences;
	Maybe<VisibilitySpec> visibSpec;

	PrerunExpression* defineChecker;

  public:
	DefinePrerunFunction(Identifier _name, Type* _returnType, Vec<Argument*> _arguments,
	                     PrerunExpression* _defineChecker, Vec<PrerunSentence*> _sentences,
	                     Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange)
	    : IsEntity(std::move(_fileRange)), name(std::move(_name)), returnType(_returnType),
	      arguments(std::move(_arguments)), sentences(std::move(_sentences)), visibSpec(_visibSpec),
	      defineChecker(_defineChecker) {}

	useit static DefinePrerunFunction* create(Identifier name, Type* returnType, Vec<Argument*> arguments,
	                                          PrerunExpression* defineChecker, Vec<PrerunSentence*> sentences,
	                                          Maybe<VisibilitySpec> visibSpec, FileRange fileRange) {
		return std::construct_at(OwnNormal(DefinePrerunFunction), std::move(name), returnType, std::move(arguments),
		                         defineChecker, std::move(sentences), visibSpec, std::move(fileRange));
	}

	void create_function(EmitCtx* ctx);

	void create_entity(ir::Mod* mod, ir::Ctx* irCtx) final;

	void update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) final;

	void do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) final;

	useit Json to_json() const final {
		return Json()._("nodeType", "definePrerunFunction")._("name", name)._("fileRange", fileRange);
	}

	useit NodeType nodeType() const final { return NodeType::DEFINE_PRERUN_FUNCTION; }
};

} // namespace qat::ast

#endif
