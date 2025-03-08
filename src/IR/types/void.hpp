#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class VoidType : public Type {
  public:
	explicit VoidType(llvm::LLVMContext& llctx);

	useit static VoidType* get(llvm::LLVMContext& llctx) {
		for (auto* typ : allTypes) {
			if (typ->type_kind() == TypeKind::VOID) {
				return (VoidType*)typ;
			}
		}
		return std::construct_at(OwnNormal(VoidType), llctx);
	}

	useit bool is_trivially_copyable() const final { return true; }

	useit bool is_trivially_movable() const final { return true; }

	useit TypeKind type_kind() const final { return TypeKind::VOID; }

	useit String to_string() const final { return "void"; }
};

} // namespace qat::ir

#endif
