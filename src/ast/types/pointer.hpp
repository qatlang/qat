#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "../../IR/types/pointer.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

enum class PtrOwnType {
  heap,
  type,
  typeParent,
  function,
  anonymous,
  region,
  anyRegion,
};

class PointerType final : public Type {
  Type*             type;
  PtrOwnType        ownTyp;
  Maybe<ast::Type*> ownerTyTy;
  bool              isMultiPtr;
  bool              isSubtypeVar;
  bool              isNonNullable;

  useit ir::PointerOwner getPointerOwner(EmitCtx* ctx, Maybe<ir::Type*> ownerVal) const;
  useit String           pointerOwnerToString() const;

public:
  PointerType(Type* _type, bool _isSubtypeVar, PtrOwnType _ownTy, bool _isNonNullable, Maybe<Type*> _ownerTyTy,
              bool _isMultiPtr, FileRange _fileRange)
      : Type(_fileRange), type(_type), ownTyp(_ownTy), ownerTyTy(_ownerTyTy), isMultiPtr(_isMultiPtr),
        isSubtypeVar(_isSubtypeVar), isNonNullable(_isNonNullable) {}

  useit static inline PointerType* create(Type* _type, bool _isSubtypeVar, PtrOwnType _ownTy, bool _isNonNullable,
                                          Maybe<Type*> _ownerTyTy, bool _isMultiPtr, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PointerType), _type, _isSubtypeVar, _ownTy, _isNonNullable, _ownerTyTy,
                             _isMultiPtr, _fileRange);
  }

  void update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent, EmitCtx* ctx) final;

  useit Maybe<usize> getTypeSizeInBits(EmitCtx* ctx) const final;

  useit ir::Type*   emit(EmitCtx* ctx) final;
  useit AstTypeKind type_kind() const final;
  useit Json        to_json() const final;
  useit String      to_string() const final;
};

} // namespace qat::ast

#endif