#ifndef QAT_PRERUN_MEMBER_FUNCTION_CALL_HPP
#define QAT_PRERUN_MEMBER_FUNCTION_CALL_HPP

#include "../expression.hpp"

namespace qat::ast {

class PrerunMemberFnCall : public PrerunExpression {
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

  useit IR::PrerunValue* emit(IR::Context* ctx) final;
  useit Json             toJson() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_MEMBER_FUNCTION_CALL; }
};

} // namespace qat::ast

#endif