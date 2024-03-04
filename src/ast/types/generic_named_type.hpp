#ifndef QAT_AST_EXPRESSIONS_GENERIC_NAMED_TYPE_HPP
#define QAT_AST_EXPRESSIONS_GENERIC_NAMED_TYPE_HPP

#include "../expression.hpp"
#include "../generics.hpp"
#include "../types/qat_type.hpp"
#include "type_kind.hpp"

namespace qat::ast {

class GenericNamedType final : public Type {
private:
  u32               relative;
  Vec<Identifier>   names;
  Vec<FillGeneric*> genericTypes;

public:
  GenericNamedType(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes, FileRange _fileRange)
      : Type(_fileRange), relative(_relative), names(_names), genericTypes(_genericTypes) {}

  useit static inline GenericNamedType* create(u32 _relative, Vec<Identifier> _names, Vec<FillGeneric*> _genericTypes,
                                               FileRange _fileRange) {
    return std::construct_at(OwnNormal(GenericNamedType), _relative, _names, _genericTypes, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) final;

  useit ir::Type*   emit(EmitCtx* ctx) final;
  useit Json        to_json() const final;
  useit String      to_string() const final;
  useit AstTypeKind type_kind() const final { return AstTypeKind::GENERIC_NAMED; }
};

Maybe<ir::Type*> handle_generic_named_type(ir::Mod* mod, ir::Block* curr, Identifier entityName, Vec<Identifier> names,
                                           Vec<FillGeneric*> genericTypes, AccessInfo reqInfo, FileRange fileRange,
                                           EmitCtx* ctx);

} // namespace qat::ast

#endif