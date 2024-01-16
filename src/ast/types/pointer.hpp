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

class PointerType : public QatType {
  QatType*             type;
  PtrOwnType           ownTyp;
  Maybe<ast::QatType*> ownerTyTy;
  bool                 isMultiPtr;
  bool                 isSubtypeVar;
  bool                 isNonNullable;

  useit IR::PointerOwner getPointerOwner(IR::Context* ctx, Maybe<IR::QatType*> ownerVal) const;
  useit String           pointerOwnerToString() const;

public:
  PointerType(QatType* _type, bool _isSubtypeVar, PtrOwnType _ownTy, bool _isNonNullable, Maybe<QatType*> _ownerTyTy,
              bool _isMultiPtr, FileRange _fileRange)
      : QatType(_fileRange), type(_type), ownTyp(_ownTy), ownerTyTy(_ownerTyTy), isMultiPtr(_isMultiPtr),
        isSubtypeVar(_isSubtypeVar), isNonNullable(_isNonNullable) {}

  useit static inline PointerType* create(QatType* _type, bool _isSubtypeVar, PtrOwnType _ownTy, bool _isNonNullable,
                                          Maybe<QatType*> _ownerTyTy, bool _isMultiPtr, FileRange _fileRange) {
    return std::construct_at(OwnNormal(PointerType), _type, _isSubtypeVar, _ownTy, _isNonNullable, _ownerTyTy,
                             _isMultiPtr, _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif