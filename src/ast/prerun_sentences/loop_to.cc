#include "./loop_to.hpp"
#include "../../IR/types/unsigned.hpp"
#include "./internal_exceptions.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/Constants.h>

// I LOVE EXCEPTIONS from now on...
#define PRERUN_LOOP_BASIC_CONTENTS                                                                                     \
	try {                                                                                                              \
		for (auto* snt : sentences) {                                                                                  \
			snt->emit(ctx);                                                                                            \
		}                                                                                                              \
	} catch (InternalPrerunBreak & brk) {                                                                              \
		if (tag.has_value() && ((not brk.tag.has_value()) || (brk.tag.value() == tag->value))) {                       \
			break;                                                                                                     \
		} else {                                                                                                       \
			throw brk;                                                                                                 \
		}                                                                                                              \
	} catch (InternalPrerunContinue & cont) {                                                                          \
		if (tag.has_value() && ((not cont.tag.has_value()) || (cont.tag.value() == tag->value))) {                     \
			continue;                                                                                                  \
		} else {                                                                                                       \
			throw cont;                                                                                                \
		}                                                                                                              \
	} catch (...) {                                                                                                    \
		throw;                                                                                                         \
	}

namespace qat::ast {

void PrerunLoopTo::emit(EmitCtx* ctx) {
	auto countExp = count->emit(ctx);
	auto countTy  = countExp->get_ir_type();

	if (countTy->is_underlying_type_integer() || countTy->is_underlying_type_unsigned()) {
		if (tag.has_value() && not ctx->get_pre_call_state()->loopsInfo.empty()) {
			for (auto& inf : ctx->get_pre_call_state()->loopsInfo) {
				if (inf.tag.has_value() && inf.tag->value == tag->value) {
					ctx->Error("Tag already used in another " + ctx->color(inf.kind_to_string()), tag->range,
					           Pair<String, FileRange>{"The existing tag can be found here", inf.tag->range});
				}
			}
		}
		ctx->get_pre_call_state()->loopsInfo.push_back(ir::PreLoopInfo{.kind = ir::PreLoopKind::TO, .tag = tag});
		if (countTy->is_integer() && (countTy->as_integer()->get_bitwidth() <= 64u) && (not tag.has_value())) {
			auto countVal = *reinterpret_cast<i64 const*>(
			    llvm::cast<llvm::ConstantInt>(countExp->get_llvm_constant())->getValue().getRawData());
			for (i64 i = 0; i < countVal; i++) {
				PRERUN_LOOP_BASIC_CONTENTS
			}
		} else if (countTy->is_unsigned_integer() && (countTy->as_unsigned_integer()->get_bitwidth() <= 64u) &&
		           (not tag.has_value())) {
			auto countVal = *llvm::cast<llvm::ConstantInt>(countExp->get_llvm_constant())->getValue().getRawData();
			for (u64 i = 0; i < countVal; i++) {
				PRERUN_LOOP_BASIC_CONTENTS
			}
		} else {
			const bool isUnsigned = countTy->is_underlying_type_unsigned();
			for (auto index = llvm::ConstantInt::get(countTy->get_llvm_type(), 0u, not isUnsigned);
			     llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
			                                       isUnsigned ? llvm::CmpInst::ICMP_ULT : llvm::CmpInst::ICMP_SLT,
			                                       index, countExp->get_llvm_constant()))
			         ->getValue()
			         .getBoolValue();
			     index = llvm::ConstantFoldConstant(
			         llvm::ConstantExpr::getAdd(index,
			                                    llvm::ConstantInt::get(countTy->get_llvm_type(), 1u, not isUnsigned)),
			         ctx->irCtx->dataLayout.value())) {
				PRERUN_LOOP_BASIC_CONTENTS
			}
		}
		ctx->get_pre_call_state()->loopsInfo.pop_back();
	} else {
		ctx->Error("Count of the " + ctx->color("loop to") + " is required to be of signed or unsigned integer type. " +
		               "The provided expression is of type " + ctx->color(countTy->to_string()),
		           count->fileRange);
	}
}

} // namespace qat::ast
