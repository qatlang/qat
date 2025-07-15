#ifndef QAT_PRERUN_MEMBER_FUNCTION_CALL_HPP
#define QAT_PRERUN_MEMBER_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunMemberFnCall final : public PrerunExpression {
	PrerunExpression*      instance;
	Identifier             memberName;
	Vec<PrerunExpression*> arguments;

  public:
	PrerunMemberFnCall(PrerunExpression* _instance, Identifier _memberName, Vec<PrerunExpression*> _arguments,
	                   FileRangePtr _fileRange)
	    : PrerunExpression(_fileRange), instance(_instance), memberName(_memberName), arguments(_arguments) {}

	useit static PrerunMemberFnCall* create(PrerunExpression* instance, Identifier memberName,
	                                        Vec<PrerunExpression*> arguments, FileRangePtr fileRange) {
		return std::construct_at(OwnNormal(PrerunMemberFnCall), instance, memberName, arguments, fileRange);
	}

	void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType>, ir::EntityState* ent, EmitCtx* ctx) final {
		UPDATE_DEPS(instance);
		for (auto arg : arguments) {
			UPDATE_DEPS(arg);
		}
	}

	useit ir::PrerunValue* emit(EmitCtx* ctx) final;
	useit Json             to_json() const final;
	useit String           to_string() const final;

	useit NodeType nodeType() const final { return NodeType::PRERUN_METHOD_CALL; }
};

useit ir::PrerunValue* handle_type_wrap_functions(ir::PrerunValue* typed, Vec<Expression*> const& arguments,
                                                  Identifier memberName, EmitCtx* ctx, FileRangePtr fileRange);
useit ir::PrerunValue* handle_type_wrap_functions(ir::PrerunValue* typed, Vec<PrerunExpression*> const& arguments,
                                                  Identifier memberName, EmitCtx* ctx, FileRangePtr fileRange);

} // namespace qat::ast

#endif
