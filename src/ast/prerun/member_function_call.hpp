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
                     FileRange _fileRange)
      : PrerunExpression(_fileRange), instance(_instance), memberName(_memberName), arguments(_arguments) {}

  useit static inline PrerunMemberFnCall* create(PrerunExpression* instance, Identifier memberName,
                                                 Vec<PrerunExpression*> arguments, FileRange fileRange) {
    return std::construct_at(OwnNormal(PrerunMemberFnCall), instance, memberName, arguments, fileRange);
  }

  void update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                           IR::Context* ctx) final {
    UPDATE_DEPS(instance);
    for (auto arg : arguments) {
      UPDATE_DEPS(arg);
    }
  }

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit String           toString() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_MEMBER_FUNCTION_CALL; }
};

useit IR::PrerunValue* handle_type_wrap_functions(IR::TypedType* typed, Vec<Expression*> const& arguments,
                                                  Identifier memberName, IR::Context* ctx, FileRange fileRange);
useit IR::PrerunValue* handle_type_wrap_functions(IR::TypedType* typed, Vec<PrerunExpression*> const& arguments,
                                                  Identifier memberName, IR::Context* ctx, FileRange fileRange);

} // namespace qat::ast

#endif