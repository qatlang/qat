#ifndef QAT_AST_PRERUN_EXPRESSIONS_FUNCTION_CALL_HPP
#define QAT_AST_PRERUN_EXPRESSIONS_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunFunctionCall : public PrerunExpression {
	PrerunExpression*      funcExp;
	Vec<PrerunExpression*> arguments; // TODO - Support named arguments

  public:
	PrerunFunctionCall(PrerunExpression* _funcExp, Vec<PrerunExpression*> _arguments, FileRangePtr _fileRange)
	    : PrerunExpression(std::move(_fileRange)), funcExp(_funcExp), arguments(std::move(_arguments)) {}

	static PrerunFunctionCall* create(PrerunExpression* function, Vec<PrerunExpression*> arguments,
	                                  FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunFunctionCall), function, std::move(arguments), std::move(fileRange));
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(funcExp);
		for (auto arg : arguments) {
			UPDATE_DEPS(arg);
		}
	}

	ir::PrerunValue* emit(EmitCtx* ctx) final;

	NodeType nodeType() const final { return NodeType::PRERUN_FUNCTION_CALL; }

	String to_string() const final {
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
