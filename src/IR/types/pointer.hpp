#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>

namespace qat::IR {

class Function;
class Region;

enum class PointerOwnerType {
  region,
  heap,
  anonymous,
  type,
  parentFunction,
  parentInstance,
  Static,
};

class PointerOwner {
public:
  void*            owner;
  PointerOwnerType ownerTy;

  useit static PointerOwner OfHeap();
  useit static PointerOwner OfAnonymous();
  useit static PointerOwner OfType(QatType* type);
  useit static PointerOwner OfParentFunction(Function* fun);
  useit static PointerOwner OfParentInstance(QatType* type);
  useit static PointerOwner OfRegion(Region* region);

  useit inline QatType*  ownerAsType() const { return (QatType*)owner; }
  useit inline Region*   ownerAsRegion() const { return ((QatType*)owner)->asRegion(); }
  useit inline Function* ownerAsParentFunction() const { return (Function*)owner; }
  useit inline QatType*  ownerAsParentType() const { return (QatType*)owner; }

  useit inline bool isType() const { return ownerTy == PointerOwnerType::type; }
  useit inline bool isAnonymous() const { return ownerTy == PointerOwnerType::anonymous; }
  useit inline bool isRegion() const { return ownerTy == PointerOwnerType::region; }
  useit inline bool isHeap() const { return ownerTy == PointerOwnerType::heap; }
  useit inline bool isParentFunction() const { return ownerTy == PointerOwnerType::parentFunction; }
  useit inline bool isParentInstance() const { return ownerTy == PointerOwnerType::parentInstance; }
  useit inline bool isStatic() const { return ownerTy == PointerOwnerType::Static; }

  useit bool isSame(const PointerOwner& other) const;

  useit String toString() const;
};

class PointerType : public QatType {
private:
  QatType*     subType;
  bool         isSubtypeVar;
  PointerOwner owner;
  bool         hasMulti;
  bool         nonNullable;

  PointerType(bool _isSubVar, QatType* _subtype, bool nonNullable, PointerOwner _owner, bool _hasMulti,
              IR::Context* ctx);

public:
  useit static PointerType* get(bool _isSubtypeVariable, QatType* _type, bool _nonNullable, PointerOwner _owner,
                                bool _hasMulti, IR::Context* ctx);

  useit QatType*     getSubType() const;
  useit PointerOwner getOwner() const;
  useit bool         isSubtypeVariable() const;
  useit bool         isMulti() const;
  useit bool         isNullable() const;
  useit bool         isNonNullable() const;
  useit bool         isTypeSized() const;
  useit TypeKind     typeKind() const override;
  useit String       toString() const override;
};

} // namespace qat::IR

#endif
