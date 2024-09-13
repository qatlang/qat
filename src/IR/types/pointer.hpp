#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./qat_type.hpp"

namespace qat::ir {

class Function;
class Region;

enum class PointerOwnerType {
  anyRegion,
  region,
  heap,
  anonymous,
  type,
  parentFunction,
  parentInstance,
  Static,
};

class MarkOwner {
public:
  void*            owner;
  PointerOwnerType ownerTy;

  useit static MarkOwner OfHeap();
  useit static MarkOwner OfAnonymous();
  useit static MarkOwner OfType(Type* type);
  useit static MarkOwner OfParentFunction(Function* fun);
  useit static MarkOwner OfParentInstance(Type* type);
  useit static MarkOwner OfRegion(Region* region);
  useit static MarkOwner OfAnyRegion();

  useit inline Type*     ownerAsType() const { return (Type*)owner; }
  useit inline Region*   ownerAsRegion() const { return ((Type*)owner)->as_region(); }
  useit inline Function* owner_as_parent_function() const { return (Function*)owner; }
  useit inline Type*     ownerAsParentType() const { return (Type*)owner; }

  useit inline bool isType() const { return ownerTy == PointerOwnerType::type; }
  useit inline bool isAnonymous() const { return ownerTy == PointerOwnerType::anonymous; }
  useit inline bool isAnyRegion() const { return ownerTy == PointerOwnerType::anyRegion; }
  useit inline bool isRegion() const { return ownerTy == PointerOwnerType::region; }
  useit inline bool isHeap() const { return ownerTy == PointerOwnerType::heap; }
  useit inline bool is_parent_function() const { return ownerTy == PointerOwnerType::parentFunction; }
  useit inline bool isParentInstance() const { return ownerTy == PointerOwnerType::parentInstance; }
  useit inline bool isStatic() const { return ownerTy == PointerOwnerType::Static; }

  useit bool is_same(const MarkOwner& other) const;

  useit String to_string() const;
};

class MarkType : public Type {
private:
  Type*     subType;
  bool      isSubtypeVar;
  MarkOwner owner;
  bool      hasMulti;
  bool      nonNullable;

  MarkType(bool _isSubVar, Type* _subtype, bool nonNullable, MarkOwner _owner, bool _hasMulti, ir::Ctx* irCtx);

public:
  useit static MarkType* get(bool _isSubtypeVariable, Type* _type, bool _nonNullable, MarkOwner _owner, bool _hasMulti,
                             ir::Ctx* irCtx);

  useit Type*     get_subtype() const;
  useit MarkOwner getOwner() const;

  useit bool isSubtypeVariable() const;
  useit bool isSlice() const;
  useit bool isNullable() const;
  useit bool isNonNullable() const;
  useit bool can_be_prerun() const final { return subType->is_function(); }
  useit bool is_type_sized() const final;
  useit bool has_prerun_default_value() const final;
  useit bool is_trivially_copyable() const final;
  useit bool is_trivially_movable() const final;

  useit PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

  useit TypeKind type_kind() const override;
  useit String   to_string() const override;
};

} // namespace qat::ir

#endif
