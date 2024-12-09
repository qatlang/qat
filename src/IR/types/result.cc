#include "./result.hpp"
#include "../context.hpp"

namespace qat::ir {

ResultType::ResultType(ir::Type* _resTy, ir::Type* _errTy, bool _isPacked, ir::Ctx* irCtx)
	: validType(_resTy), errorType(_errTy), isPacked(_isPacked) {
	const usize validTypeSize =
		validType->is_type_sized()
			? ((validType->is_opaque() && !validType->as_opaque()->has_subtype())
				   ? validType->as_opaque()->get_deduced_size()
				   : ((usize)(irCtx->dataLayout.value().getTypeAllocSizeInBits(validType->get_llvm_type()))))
			: 8u;
	const usize errorTypeSize =
		errorType->is_type_sized()
			? ((errorType->is_opaque() && !errorType->as_opaque()->has_subtype())
				   ? errorType->as_opaque()->get_deduced_size()
				   : ((usize)(irCtx->dataLayout.value().getTypeAllocSizeInBits(errorType->get_llvm_type()))))
			: 8u;
	linkingName = "qat'result:[" + String(isPacked ? "pack," : "") + validType->get_name_for_linking() + "," +
				  errorType->get_name_for_linking() + "]";
	const auto dataType =
		(validType->is_same(errorType) && !validType->is_opaque())
			? validType->get_llvm_type()
			: llvm::Type::getIntNTy(irCtx->llctx, (validTypeSize > errorTypeSize) ? validTypeSize : errorTypeSize);
	if (is_identical_to_bool()) {
		llvmType = llvm::Type::getInt1Ty(irCtx->llctx);
	} else {
		llvmType = llvm::StructType::create(irCtx->llctx, {llvm::Type::getInt1Ty(irCtx->llctx), dataType}, linkingName,
											isPacked);
	}
}

ResultType* ResultType::get(ir::Type* validType, ir::Type* errorType, bool isPacked, ir::Ctx* irCtx) {
	for (auto typ : allTypes) {
		if (typ->is_result() && typ->as_result()->getValidType()->is_same(validType) &&
			typ->as_result()->getErrorType()->is_same(errorType) && (typ->as_result()->isPacked == isPacked)) {
			return typ->as_result();
		}
	}
	return std::construct_at(OwnNormal(ResultType), validType, errorType, isPacked, irCtx);
}

ir::Type* ResultType::getValidType() const { return validType; }

ir::Type* ResultType::getErrorType() const { return errorType; }

void ResultType::handle_tag_store(llvm::Value* resultRef, bool tag, ir::Ctx* irCtx) {
	if (is_identical_to_bool()) {
		irCtx->builder.CreateStore(llvm::ConstantInt::getBool(irCtx->llctx, tag), resultRef);
	} else {
		irCtx->builder.CreateStore(llvm::ConstantInt::getBool(irCtx->llctx, tag),
								   irCtx->builder.CreateStructGEP(llvmType, resultRef, 0u));
	}
}

bool ResultType::isTypePacked() const { return isPacked && !is_identical_to_bool(); }

String ResultType::to_string() const {
	return "result:[" + validType->to_string() + ", " + errorType->to_string() + "]";
}

} // namespace qat::ir
