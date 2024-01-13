#ifndef QAT_TYPES_POINTER_HPP
#define QAT_TYPES_POINTER_HPP

#include "../../IR/types/pointer.hpp"
#include "./qat_type.hpp"
#include "./void.hpp"
#include <string>

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
private:
  QatType*             type;
  PtrOwnType           ownTyp;
  Maybe<ast::QatType*> ownerTyTy;
  bool                 isMultiPtr;
  bool                 isSubtypeVar;
  bool                 isNonNullable;

  useit IR::PointerOwner getPointerOwner(IR::Context* ctx, Maybe<IR::QatType*> ownerVal) const;
  useit String           pointerOwnerToString() const;

public:
  PointerType(QatType* _type, bool isSubtypeVar, PtrOwnType _ownTy, bool _isNonNullable, Maybe<QatType*> _ownerTyTy,
              bool _isMultiPtr, FileRange _fileRange);

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif