#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class VoidType : public Type {
	static VoidType* voidType;

  public:
	explicit VoidType(llvm::LLVMContext& llctx);

	static VoidType* get(llvm::LLVMContext& llctx) {
		if (voidType) {
			return voidType;
		}
		return std::construct_at(OwnNormal(VoidType), llctx);
	}

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	TypeKind type_kind() const final { return TypeKind::VOID; }

	String to_string() const final { return "void"; }
};

} // namespace qat::ir

#endif
