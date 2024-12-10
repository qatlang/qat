#ifndef QAT_AST_PRERUN_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunFunctionCall : public PrerunExpression {
	PrerunExpression*      funcExp;
	Vec<PrerunExpression*> arguments; // TODO - Support named arguments

  public:
	PrerunFunctionCall(PrerunExpression* _funcExp, Vec<PrerunExpression*> _arguments, FileRange _fileRange)
	    : PrerunExpression(std::move(_fileRange)), funcExp(_funcExp), arguments(std::move(_arguments)) {}

	useit static PrerunFunctionCall* create(PrerunExpression* function, Vec<PrerunExpression*> arguments,
	                                        FileRange fileRange) {
		return std::construct_at(OwnNormal(PrerunFunctionCall), function, std::move(arguments), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(funcExp);
		for (auto arg : arguments) {
			UPDATE_DEPS(arg);
		}
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;

	useit NodeType nodeType() const final { return NodeType::PRERUN_FUNCTION_CALL; }

	useit Json to_json() const final {
		Vec<JsonValue> argsJSON;
		for (auto arg : arguments) {
			argsJSON.push_back(arg->to_json());
		}
		return Json()
		    ._("nodeType", "prerunFunctionCall")
		    ._("function", funcExp->to_json())
		    ._("arguments", std::move(argsJSON))
		    ._("fileRange", fileRange);
	}

	useit String to_string() const final {
		String result = funcExp->to_string() + "(";
		for (usize i = 0; i < arguments.size(); i++) {
			result += arguments.at(i)->to_string();
			if (i != (arguments.size() - 1)) {
				result += ", ";
			}
		}
		result += ")";
		return result;
	}
};

} // namespace qat::ast

#endif
