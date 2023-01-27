#ifndef QAT_IR_TYPES_OPAQUE_HPP
#define QAT_IR_TYPES_OPAQUE_HPP

#include "../../memory_tracker.hpp"
#include "../../utils/identifier.hpp"
#include "qat_type.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::ast {
class DefineCoreType;
class DefineMixType;
} // namespace qat::ast

namespace qat::IR {

class QatModule;
class MemberFunction;

class OpaqueType : public QatType {
  friend class ast::DefineCoreType;
  friend class ast::DefineMixType;

  Identifier          name;
  Maybe<String>       genericVariantName;
  IR::QatModule*      parent;
  IR::ExpandedType*   subTy      = nullptr;
  IR::MemberFunction* destructor = nullptr;
  Maybe<usize>        size;

  OpaqueType(Identifier _name, Maybe<String> genericVariantName, IR::QatModule* _parent, Maybe<usize> _size,
             llvm::LLVMContext& llctx);

public:
  useit static OpaqueType* get(Identifier name, Maybe<String> genericVariantName, IR::QatModule* parent,
                               Maybe<usize> size, llvm::LLVMContext& llCtx);

  useit String     getFullName() const;
  useit Identifier getName() const;
  useit bool       hasSubType() const;
  void             setSubType(IR::ExpandedType* _subTy);
  useit QatType*   getSubType() const;
  useit bool       hasSize() const;

  useit bool     isDestructible() const final;
  void           destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
  useit TypeKind typeKind() const final;
  useit String   toString() const final;
  useit Json     toJson() const final { return {}; }
};

} // namespace qat::IR

#endif