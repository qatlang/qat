#ifndef QAT_IR_TYPES_OPAQUE_HPP
#define QAT_IR_TYPES_OPAQUE_HPP

#include "../../memory_tracker.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../generics.hpp"
#include "qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ast {
class DefineCoreType;
class DefineMixType;
} // namespace qat::ast

namespace qat::IR {

class QatModule;
class MemberFunction;

enum class OpaqueSubtypeKind { core, mix, unknown };

class OpaqueType : public EntityOverview, public QatType {
  friend class ast::DefineCoreType;
  friend class ast::DefineMixType;

  Identifier               name;
  Vec<GenericParameter*>   generics;
  Maybe<String>            genericID;
  Maybe<OpaqueSubtypeKind> subtypeKind;
  IR::QatModule*           parent;
  IR::ExpandedType*        subTy = nullptr;
  Maybe<usize>             size;
  VisibilityInfo           visibility;

  OpaqueType(Identifier _name, Vec<GenericParameter*> _generics, Maybe<String> _genericID,
             Maybe<OpaqueSubtypeKind> subtypeKind, IR::QatModule* _parent, Maybe<usize> _size,
             VisibilityInfo _visibility, llvm::LLVMContext& llctx);

public:
  useit static OpaqueType* get(Identifier name, Vec<GenericParameter*> generics, Maybe<String> genericID,
                               Maybe<OpaqueSubtypeKind> subtypeKind, IR::QatModule* parent, Maybe<usize> size,
                               VisibilityInfo visibility, llvm::LLVMContext& llCtx);

  useit String                getFullName() const;
  useit Identifier            getName() const;
  useit VisibilityInfo const& getVisibility() const;
  useit bool                  isGeneric() const;
  useit Maybe<String>     getGenericID() const;
  useit bool              hasGenericParameter(String const& name) const;
  useit GenericParameter* getGenericParameter(String const& name) const;

  useit bool     isSubtypeCore() const;
  useit bool     isSubtypeMix() const;
  useit bool     isSubtypeUnknown() const;
  useit bool     hasSubType() const;
  void           setSubType(IR::ExpandedType* _subTy);
  useit QatType* getSubType() const;
  useit bool     hasDeducedSize() const;
  useit usize    getDeducedSize() const;

  useit bool isExpanded() const final;
  useit bool hasNoValueSemantics() const final;
  useit bool canBePrerunGeneric() const final;
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* preVal) const final;
  useit bool          isTypeSized() const final;
  useit Maybe<bool> equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const final;

  useit bool isCopyConstructible() const final;
  useit bool isCopyAssignable() const final;
  useit bool isMoveConstructible() const final;
  useit bool isMoveAssignable() const final;
  useit bool isDestructible() const final;

  void copyConstructValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
  void copyAssignValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
  void moveConstructValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
  void moveAssignValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
  void destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;

  useit TypeKind typeKind() const final;
  useit String   toString() const final;
};

} // namespace qat::IR

#endif