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
  function,
};

class PointerOwner {
public:
  void*            owner;
  PointerOwnerType ownerTy;

  useit static PointerOwner OfHeap();
  useit static PointerOwner OfAnonymous();
  useit static PointerOwner OfType(QatType* type);
  useit static PointerOwner OfFunction(Function* fun);
  // TODO - Add region
  useit static PointerOwner OfRegion(Region* region);

  useit QatType*  ownerAsType() const;
  useit Region*   ownerAsRegion() const;
  useit Function* ownerAsFunction() const;

  useit bool isType() const;
  useit bool isAnonymous() const;
  useit bool isRegion() const;
  useit bool isHeap() const;
  useit bool isFunction() const;

  useit bool isSame(const PointerOwner& other) const;

  useit String toString() const;
};

class PointerType : public QatType {
private:
  QatType*     subType;
  bool         isSubtypeVar;
  PointerOwner owner;
  bool         hasMulti;

  PointerType(bool _isSubVar, QatType* _subtype, PointerOwner _owner, bool _hasMulti, IR::Context* ctx);

public:
  useit static PointerType* get(bool _isSubtypeVariable, QatType* _type, PointerOwner _owner, bool _hasMulti,
                                IR::Context* ctx);

  useit QatType*     getSubType() const;
  useit PointerOwner getOwner() const;
  useit bool         isSubtypeVariable() const;
  useit bool         isMulti() const;
  useit TypeKind     typeKind() const override;
  useit String       toString() const override;
};

} // namespace qat::IR

#endif
