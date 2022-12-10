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
  parent,
  anonymous,
  region,
};

class PointerType : public QatType {
private:
  QatType*             type;
  PtrOwnType           ownTyp;
  Maybe<ast::QatType*> ownerTyTy;
  bool                 isMultiPtr;

  useit IR::PointerOwner getPointerOwner(IR::Context* ctx, Maybe<IR::QatType*> ownerVal) const;

public:
  PointerType(QatType* _type, bool _variable, PtrOwnType _ownTy, Maybe<QatType*> _ownerTyTy, bool _isMultiPtr,
              FileRange _fileRange);

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif