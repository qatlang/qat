#ifndef QAT_AST_META_MATH_HPP
#define QAT_AST_META_MATH_HPP

#include "../expression.hpp"

namespace qat::parser {
class Parser;
}

namespace qat::ast {

enum class MathUnit {
	ABS,
	SMAX,
};

class MetaMath : public Expression {
	friend class parser::Parser;

	Identifier       name;
	Vec<Expression*> arguments;

	static const std::set<String> functionNames;

  public:
	MetaMath(Identifier _name, Vec<Expression*> _arguments, FileRangePtr _fileRange)
	    : Expression(_fileRange), name(_name), arguments(std::move(_arguments)) {}

	useit static MetaMath* create(Identifier name, Vec<Expression*> arguments, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(MetaMath), std::move(name), std::move(arguments), fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* state,
	                         EmitCtx* ctx) final {
		for (auto arg : arguments) {
			arg->update_dependencies(phase, dep, state, ctx);
		}
	}

	useit ir::Value* emit(EmitCtx* ctx) final;

	useit Json to_json() const final {
		Vec<JsonValue> argsJSON;
		for (auto arg : arguments) {
			argsJSON.push_back(arg->to_json());
		}
		return Json()
		    ._("nodeType", "metaMath")
		    ._("name", name)
		    ._("arguments", argsJSON)
		    ._("fileRange", fileRange->to_json_value());
	}

	useit NodeType nodeType() const final { return NodeType::META_MATH; }
};

} // namespace qat::ast

#endif
