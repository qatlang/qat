#include "./vector.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

VectorType::VectorType(ir::Type* _subType, usize _count, VectorKind _kind, ir::Ctx* irCtx)
	: subType(_subType), count(_count), kind(_kind) {
	llvmType = kind == VectorKind::fixed ? (llvm::Type*)llvm::FixedVectorType::get(subType->get_llvm_type(), count)
										 : llvm::ScalableVectorType::get(subType->get_llvm_type(), count);
}

VectorType* VectorType::create(ir::Type* subType, usize count, VectorKind kind, ir::Ctx* irCtx) {
	for (auto typ : allTypes) {
		if (typ->type_kind() == TypeKind::vector) {
			if (((VectorType*)typ)->subType->is_same(subType) && (((VectorType*)typ)->count == count) &&
				(((VectorType*)typ)->kind == kind)) {
				return (VectorType*)typ;
			}
		}
	}
	return new VectorType(subType, count, kind, irCtx);
}

String VectorType::to_string() const {
	return String("vec:[") + ((kind == VectorKind::scalable) ? "?, " : "") + std::to_string(count) + ", " +
		   subType->to_string() + "]";
}

} // namespace qat::ir
