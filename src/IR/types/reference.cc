#include "./reference.hpp"
#include "../context.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"

namespace qat::ir {

ReferenceType::ReferenceType(bool _isSubtypeVariable, Type* _type, ir::Ctx* irCtx)
    : subType(_type), isSubVariable(_isSubtypeVariable) {
  if (subType->is_type_sized()) {
    llvmType = llvm::PointerType::get(subType->get_llvm_type(), irCtx->dataLayout->getProgramAddressSpace());
  } else {
    llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), irCtx->dataLayout->getProgramAddressSpace());
  }
  linkingName = "qat'@" + String(isSubVariable ? "var[" : "[") + subType->get_name_for_linking() + "]";
}

ReferenceType* ReferenceType::get(bool _isSubtypeVariable, Type* _subtype, ir::Ctx* irCtx) {
  for (auto* typ : allTypes) {
    if (typ->is_reference()) {
      if (typ->as_reference()->get_subtype()->is_same(_subtype) &&
          (typ->as_reference()->isSubtypeVariable() == _isSubtypeVariable)) {
        return typ->as_reference();
      }
    }
  }
  return new ReferenceType(_isSubtypeVariable, _subtype, irCtx);
}

Type* ReferenceType::get_subtype() const { return subType; }

bool ReferenceType::isSubtypeVariable() const { return isSubVariable; }

bool ReferenceType::is_type_sized() const { return true; }

TypeKind ReferenceType::type_kind() const { return TypeKind::reference; }

String ReferenceType::to_string() const {
  return "@" + String(isSubVariable ? "var[" : "[") + subType->to_string() + "]";
}

} // namespace qat::ir