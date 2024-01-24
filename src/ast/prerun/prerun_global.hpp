#ifndef QAT_AST_PRERUN_GLOBAL_HPP
#define QAT_AST_PRERUN_GLOBAL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunGlobal : public PrerunExpression {
  Identifier            name;
  Maybe<QatType*>       type;
  PrerunExpression*     value;
  Maybe<VisibilitySpec> visibSpec;

public:
  PrerunGlobal(Identifier _name, Maybe<QatType*> _type, PrerunExpression* _value, Maybe<VisibilitySpec> _visibSpec,
               FileRange _fileRange)
      : PrerunExpression(_fileRange), name(_name), type(_type), value(_value), visibSpec(_visibSpec) {}

  useit static inline PrerunGlobal* create(Identifier _name, Maybe<QatType*> _type, PrerunExpression* _value,
                                           Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunGlobal), _name, _type, _value, _visibSpec, _fileRange);
  }

  void  createModule(IR::Context* ctx) const final;
  useit IR::PrerunValue* emit(IR::Context* ctx) final { return nullptr; }
  useit Json             toJson() const final;
  useit NodeType         nodeType() const final { return NodeType::PRERUN_GLOBAL; }
};

} // namespace qat::ast

#endif
