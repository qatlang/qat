#ifndef QAT_AST_PRERUN_GLOBAL_HPP
#define QAT_AST_PRERUN_GLOBAL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunGlobal final : public IsEntity {
  Identifier            name;
  Maybe<QatType*>       type;
  PrerunExpression*     value;
  Maybe<VisibilitySpec> visibSpec;

public:
  PrerunGlobal(Identifier _name, Maybe<QatType*> _type, PrerunExpression* _value, Maybe<VisibilitySpec> _visibSpec,
               FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), type(_type), value(_value), visibSpec(_visibSpec) {}

  useit static inline PrerunGlobal* create(Identifier _name, Maybe<QatType*> _type, PrerunExpression* _value,
                                           Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunGlobal), _name, _type, _value, _visibSpec, _fileRange);
  }

  void create_entity(IR::QatModule* parent, IR::Context* ctx) final;
  void update_entity_dependencies(IR::QatModule* parent, IR::Context* ctx) final;
  void do_phase(IR::EmitPhase phase, IR::QatModule* parent, IR::Context* ctx) final;

  void           define(IR::QatModule* mod, IR::Context* ctx) const;
  useit Json     toJson() const final;
  useit NodeType nodeType() const final { return NodeType::PRERUN_GLOBAL; }
};

} // namespace qat::ast

#endif
