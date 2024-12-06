#ifndef QAT_AST_EXPRESSIONS_GENERIC_ENTITY_HPP
#define QAT_AST_EXPRESSIONS_GENERIC_ENTITY_HPP

#include "../expression.hpp"
#include "../generics.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

class GenericEntity final : public Expression {
private:
  u32               relative;
  Vec<Identifier>   names;
  Vec<FillGeneric*> genericTypes;

public:
  GenericEntity(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes, FileRange _fileRange)
      : Expression(_fileRange), relative(_relative), names(_names), genericTypes(_genericTypes) {}

  useit static GenericEntity* create(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes,
                                     FileRange _fileRange) {
    return std::construct_at(OwnNormal(GenericEntity), _relative, _names, _genericTypes, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent, EmitCtx* ctx) final;

  useit ir::Value* emit(EmitCtx* ctx) final;
  useit Json       to_json() const final;
  useit NodeType   nodeType() const final { return NodeType::GENERIC_ENTITY; }
};

} // namespace qat::ast

#endif
