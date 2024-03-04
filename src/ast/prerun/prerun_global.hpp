#ifndef QAT_AST_PRERUN_GLOBAL_HPP
#define QAT_AST_PRERUN_GLOBAL_HPP

#include "../expression.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class PrerunGlobal final : public IsEntity {
  Identifier            name;
  Maybe<Type*>          type;
  PrerunExpression*     value;
  Maybe<VisibilitySpec> visibSpec;

public:
  PrerunGlobal(Identifier _name, Maybe<Type*> _type, PrerunExpression* _value, Maybe<VisibilitySpec> _visibSpec,
               FileRange _fileRange)
      : IsEntity(_fileRange), name(_name), type(_type), value(_value), visibSpec(_visibSpec) {}

  useit static inline PrerunGlobal* create(Identifier _name, Maybe<Type*> _type, PrerunExpression* _value,
                                           Maybe<VisibilitySpec> _visibSpec, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PrerunGlobal), _name, _type, _value, _visibSpec, _fileRange);
  }

  void create_entity(ir::Mod* parent, ir::Ctx* irCtx) final;
  void update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) final;
  void do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) final;

  void           define(ir::Mod* mod, ir::Ctx* irCtx) const;
  useit Json     to_json() const final;
  useit NodeType nodeType() const final { return NodeType::PRERUN_GLOBAL; }
};

} // namespace qat::ast

#endif
