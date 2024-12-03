#include "./loop_to.hpp"
#include "./internal_exceptions.hpp"
#include "llvm/IR/Constants.h"

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
    const bool isUnsigned = countTy->is_underlying_type_unsigned();
    for (auto index = llvm::ConstantInt::get(countTy->get_llvm_type(), 0u, not isUnsigned);
         llvm::cast<llvm::ConstantInt>(
             llvm::ConstantFoldCompareInstruction(isUnsigned ? llvm::CmpInst::ICMP_ULT : llvm::CmpInst::ICMP_SLT, index,
                                                  countExp->get_llvm_constant()))
             ->getValue()
             .getBoolValue();
         index =
             llvm::ConstantExpr::getAdd(index, llvm::ConstantInt::get(countTy->get_llvm_type(), 1u, not isUnsigned))) {
      try { // I LOVE EXCEPTIONS from now on...
        for (auto snt : sentences) {
          snt->emit(ctx);
        }
      } catch (InternalPrerunBreak& brk) {
        if (tag.has_value() && ((not brk.tag.has_value()) || (brk.tag.value() == tag->value))) {
          break;
        } else {
          throw;
        }
      } catch (InternalPrerunContinue& cont) {
        if (tag.has_value() && ((not cont.tag.has_value()) || (cont.tag.value() == tag->value))) {
          continue;
        } else {
          throw;
        }
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
