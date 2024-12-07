#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "./qat_type.hpp"

namespace qat::ir {

class VoidType : public Type {
	VoidType(llvm::LLVMContext& llctx);

  public:
	useit static VoidType* get(llvm::LLVMContext& llctx) {
		for (auto* typ : allTypes) {
			if (typ->type_kind() == TypeKind::Void) {
				return (VoidType*)typ;
			}
		}
		return new VoidType(llctx);
	}
	useit bool	   is_trivially_copyable() const final { return true; }
	useit bool	   is_trivially_movable() const final { return true; }
	useit TypeKind type_kind() const final { return TypeKind::Void; }
	useit String   to_string() const final { return "void"; }
};

} // namespace qat::ir

#endif
