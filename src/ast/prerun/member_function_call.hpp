#ifndef QAT_PRERUN_MEMBER_FUNCTION_CALL_HPP
#define QAT_PRERUN_MEMBER_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PreMemberFnCall : public PrerunExpression {
  PrerunExpression*      instance;
  Identifier             memberName;
  Vec<PrerunExpression*> arguments;

  PreMemberFnCall(PrerunExpression* instance, Identifier memberName, Vec<PrerunExpression*> arguments,
                  FileRange fileRange);

public:
  static PreMemberFnCall* Create(PrerunExpression* instance, Identifier memberName, Vec<PrerunExpression*> arguments,
                                 FileRange fileRange);

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit NodeType         nodeType() const final { return NodeType::prerunMemberFunctionCall; }
};

} // namespace qat::ast

#endif