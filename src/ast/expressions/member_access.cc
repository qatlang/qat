#include "./member_access.hpp"
#include "../../IR/types/future.hpp"
#include "../../IR/types/string_slice.hpp"
#include "../../utils/helpers.hpp"
#include "llvm/Analysis/ConstantFolding.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

ir::Value* MemberAccess::emit(EmitCtx* ctx) {
  // NOLINTBEGIN(readability-magic-numbers, clang-analyzer-core.CallAndMessage)
  if (isExpSelf) {
    if (ctx->get_fn()->is_method()) {
      auto* memFn = (ir::Method*)ctx->get_fn();
      if (memFn->is_static_method()) {
        ctx->Error("This is a static member function and hence cannot access members of the parent instance",
                   fileRange);
      }
    } else {
      ctx->Error(
          "The parent function is not a member function of any type and hence cannot access members on the parent instance",
          fileRange);
    }
  } else {
    if (instance->nodeType() == NodeType::SELF) {
      ctx->Error("Do not use this syntax for accessing members of the parent instance. Use " +
                     ctx->color(String("''") +
                                (isVariationAccess.has_value() && isVariationAccess.value()
                                     ? "var:"
                                     : (isVariationAccess.has_value() ? "const:" : "")) +
                                name.value) +
                     " instead",
                 fileRange);
    }
  }
  auto* inst     = instance->emit(ctx);
  auto* instType = inst->get_ir_type();
  bool  isVar    = inst->is_variable();
  if (instType->is_reference()) {
    inst->load_ghost_pointer(ctx->irCtx->builder);
    isVar    = instType->as_reference()->isSubtypeVariable();
    instType = instType->as_reference()->get_subtype();
  }
  if (instType->is_array()) {
    if (name.value == "length") {
      return ir::PrerunValue::get(
          llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), instType->as_array()->get_length()),
          // NOLINTNEXTLINE(readability-magic-numbers)
          ir::IntegerType::get(64u, ctx->irCtx));
    } else {
      ctx->Error("Invalid name for member access " + ctx->color(name.value) + " for expression with type " +
                     ctx->color(instType->to_string()),
                 name.range);
    }
  } else if (instType->is_string_slice()) {
    if (name.value == "length") {
      if (inst->is_prerun_value()) {
        return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(1u),
                                    ir::CType::get_usize(ctx->irCtx));
      } else if (inst->is_value()) {
        return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {1u}),
                              ir::CType::get_usize(ctx->irCtx), false);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(),
                                                                  inst->get_llvm(), 1u),
                              ir::ReferenceType::get(false, ir::CType::get_usize(ctx->irCtx), ctx->irCtx), false);
      }
    } else if (name.value == "data") {
      if (inst->is_prerun_value()) {
        return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(0u),
                                    ir::PointerType::get(false, ir::UnsignedType::get(8u, ctx->irCtx), false,
                                                         ir::PointerOwner::OfAnonymous(), false, ctx->irCtx));
      } else if (inst->is_value()) {
        return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u}),
                              ir::PointerType::get(false, ir::UnsignedType::get(8u, ctx->irCtx), false,
                                                   ir::PointerOwner::OfAnonymous(), false, ctx->irCtx),
                              false);
      } else {
        SHOW("String slice is an implicit pointer or a reference or pointer")
        return ir::Value::get(
            ctx->irCtx->builder.CreateStructGEP(ir::StringSliceType::get(ctx->irCtx)->get_llvm_type(), inst->get_llvm(),
                                                0u),
            ir::ReferenceType::get(false,
                                   ir::PointerType::get(false, ir::UnsignedType::get(8u, ctx->irCtx),
                                                        false, // NOLINT(readability-magic-numbers)
                                                        ir::PointerOwner::OfAnonymous(), false, ctx->irCtx),
                                   ctx->irCtx),
            false);
      }
    } else {
      ctx->Error("Invalid name for member access: " + ctx->color(name.value) + " for expression of type " +
                     instType->to_string(),
                 name.range);
    }
  } else if (instType->is_future()) {
    // FIXME - ?? Also support values if possible
    if (inst->is_value()) {
      inst = inst->make_local(ctx, None, instance->fileRange);
    }
    if (name.value == "isDone") {
      return ir::Value::get(ctx->irCtx->builder.CreateLoad(
                                llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                                ctx->irCtx->builder.CreatePointerCast(
                                    ctx->irCtx->builder.CreateInBoundsGEP(
                                        llvm::Type::getInt64Ty(ctx->irCtx->llctx),
                                        ctx->irCtx->builder.CreateLoad(
                                            llvm::Type::getInt64Ty(ctx->irCtx->llctx)
                                                ->getPointerTo(ctx->irCtx->dataLayout.value().getProgramAddressSpace()),
                                            ctx->irCtx->builder.CreateStructGEP(instType->as_future()->get_llvm_type(),
                                                                                inst->get_llvm(), 1u)),
                                        {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
                                    llvm::Type::getInt1Ty(ctx->irCtx->llctx)
                                        ->getPointerTo(ctx->irCtx->dataLayout.value().getProgramAddressSpace())),
                                true),
                            ir::UnsignedType::getBool(ctx->irCtx), false);
    } else if (name.value == "isNotDone") {
      return ir::Value::get(
          ctx->irCtx->builder.CreateICmpEQ(
              ctx->irCtx->builder.CreateLoad(
                  llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                  ctx->irCtx->builder.CreatePointerCast(
                      ctx->irCtx->builder.CreateInBoundsGEP(
                          llvm::Type::getInt64Ty(ctx->irCtx->llctx),
                          ctx->irCtx->builder.CreateLoad(
                              llvm::Type::getInt64Ty(ctx->irCtx->llctx)
                                  ->getPointerTo(ctx->irCtx->dataLayout.value().getProgramAddressSpace()),
                              ctx->irCtx->builder.CreateStructGEP(instType->as_future()->get_llvm_type(),
                                                                  inst->get_llvm(), 1u)),
                          {llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->irCtx->llctx), 1u)}),
                      llvm::Type::getInt1Ty(ctx->irCtx->llctx)
                          ->getPointerTo(ctx->irCtx->dataLayout.value().getProgramAddressSpace())),
                  true),
              llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
          ir::UnsignedType::getBool(ctx->irCtx), false);
    } else {
      ctx->Error("Invalid name " + ctx->color(name.value) + " for member access for type " +
                     ctx->color(instType->to_string()),
                 name.range);
    }
  } else if (instType->is_maybe()) {
    if (inst->is_value() && !instType->is_trivially_movable()) {
      inst = inst->make_local(ctx, None, instance->fileRange);
    }
    if (name.value == "hasValue") {
      if (inst->is_prerun_value()) {
        return ir::PrerunValue::get(llvm::cast<llvm::ConstantInt>(inst->get_llvm_constant()->getAggregateElement(0u)),
                                    ir::UnsignedType::getBool(ctx->irCtx));
      } else if (inst->is_value()) {
        return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u}),
                              ir::UnsignedType::getBool(ctx->irCtx), false);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(
                                  ctx->irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                                                                 ctx->irCtx->builder.CreateStructGEP(
                                                                     instType->get_llvm_type(), inst->get_llvm(), 0u)),
                                  llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 1u)),
                              ir::UnsignedType::getBool(ctx->irCtx), false);
      }
    } else if (name.value == "hasNoValue") {
      if (inst->is_prerun_value()) {
        return ir::PrerunValue::get(
            llvm::ConstantFoldConstant(
                llvm::ConstantExpr::getICmp(llvm::CmpInst::Predicate::ICMP_EQ,
                                            inst->get_llvm_constant()->getAggregateElement(0u),
                                            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
                ctx->irCtx->dataLayout.value()),
            ir::UnsignedType::getBool(ctx->irCtx));
      } else if (inst->is_value()) {
        return ir::Value::get(
            ctx->irCtx->builder.CreateNot(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {0u})),
            ir::UnsignedType::getBool(ctx->irCtx), false);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateICmpEQ(
                                  ctx->irCtx->builder.CreateLoad(llvm::Type::getInt1Ty(ctx->irCtx->llctx),
                                                                 ctx->irCtx->builder.CreateStructGEP(
                                                                     instType->get_llvm_type(), inst->get_llvm(), 0u)),
                                  llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u)),
                              ir::UnsignedType::getBool(ctx->irCtx), false);
      }
    } else {
      ctx->Error("Invalid name " + ctx->color(name.value) + " for member access of type " +
                     ctx->color(instType->to_string()),
                 fileRange);
    }
  } else if (instType->is_expanded()) {
    if ((instType->is_struct() && !instType->as_struct()->has_field_with_name(name.value)) &&
        ((isVariationAccess.has_value() && isVariationAccess.value())
             ? !instType->as_expanded()->has_variation(name.value)
             : !instType->as_expanded()->has_normal_method(name.value))) {
      ctx->Error("Core type " + ctx->color(instType->as_struct()->to_string()) +
                     " does not have a member field, member function or variation function named " +
                     ctx->color(name.value) + ". Please check the logic",
                 name.range);
    }
    auto* eTy = instType->as_expanded();
    if (eTy->is_struct() && eTy->as_struct()->has_field_with_name(name.value)) {
      if (isVariationAccess.has_value() && isVariationAccess.value()) {
        ctx->Error(ctx->color(name.value) + " is a member field of type " + ctx->color(eTy->get_full_name()) +
                       " and hence variation access cannot be used. Please change " + ctx->color("'var:" + name.value) +
                       " to " + ctx->color("'" + name.value),
                   fileRange);
      }
      auto* mem = eTy->as_struct()->get_field_at(instType->as_struct()->get_index_of(name.value).value());
      mem->add_mention(name.range);
      if (isExpSelf) {
        auto* mFn = (ir::Method*)ctx->get_fn();
        if (mFn->isConstructor()) {
          if (!mFn->is_member_initted(eTy->as_struct()->get_field_index(name.value))) {
            auto mem = eTy->as_struct()->get_field_with_name(name.value);
            if (mem->defaultValue.has_value()) {
              ctx->Error(
                  "Member field " + ctx->color(name.value) + " of parent type " + ctx->color(eTy->to_string()) +
                      " is not initialised yet and hence cannot be used. The field has a default value provided, which will be used to initialise it only at the end of this constructor",
                  fileRange);
            } else {
              ctx->Error("Member field " + ctx->color(name.value) + " of parent type " + ctx->color(eTy->to_string()) +
                             " has not been initialised yet and hence cannot be used",
                         fileRange);
            }
          }
        } else {
          mFn->add_used_members(mem->name.value);
        }
      }
      if (!mem->visibility.is_accessible(ctx->get_access_info())) {
        ctx->Error("Member " + ctx->color(name.value) + " of core type " + ctx->color(eTy->get_full_name()) +
                       " is not accessible here",
                   fileRange);
      }
      if (inst->is_value() && !instType->is_trivially_movable()) {
        inst = inst->make_local(ctx, None, instance->fileRange);
      }
      if (inst->is_prerun_value()) {
        return ir::PrerunValue::get(
            inst->get_llvm_constant()->getAggregateElement(instType->as_struct()->get_index_of(name.value).value()),
            mem->type);
      } else if (inst->is_value()) {
        return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(
                                  inst->get_llvm(), {(u32)instType->as_struct()->get_index_of(name.value).value()}),
                              instType->as_struct()->get_type_of_field(name.value), true);
      } else {
        auto llVal    = ctx->irCtx->builder.CreateStructGEP(instType->as_struct()->get_llvm_type(), inst->get_llvm(),
                                                            instType->as_struct()->get_index_of(name.value).value());
        auto memValTy = instType->as_struct()->get_type_of_field(name.value);
        if (memValTy->is_reference()) {
          llVal = ctx->irCtx->builder.CreateLoad(memValTy->get_llvm_type(), llVal);
        }
        return ir::Value::get(llVal, ir::ReferenceType::get(isVar, memValTy, ctx->irCtx), false);
      }
    } else if (!(isVariationAccess.has_value() && isVariationAccess.value()) && eTy->has_normal_method(name.value)) {
      // FIXME - Implement
      ctx->Error("Referencing member function is not supported", fileRange);
    } else if ((isVariationAccess.has_value() && isVariationAccess.value()) && eTy->has_variation(name.value)) {
      // FIXME - Implement
      ctx->Error("Referencing variation function is not supported", fileRange);
    } else {
      ctx->Error("Member access of " + ctx->color(name.value) + " is not supported for expression of type " +
                     ctx->color(instType->to_string()),
                 fileRange);
    }
  } else if (instType->is_pointer() && instType->as_pointer()->isMulti()) {
    if (name.value == "length") {
      if (inst->is_prerun_value()) {
        return ir::PrerunValue::get(inst->get_llvm_constant()->getAggregateElement(1u),
                                    ir::CType::get_usize(ctx->irCtx));
      } else if (inst->is_value()) {
        return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(inst->get_llvm(), {1u}),
                              ir::CType::get_usize(ctx->irCtx), false);
      } else {
        return ir::Value::get(ctx->irCtx->builder.CreateLoad(
                                  ir::CType::get_usize(ctx->irCtx)->get_llvm_type(),
                                  ctx->irCtx->builder.CreateStructGEP(instType->get_llvm_type(), inst->get_llvm(), 1u)),
                              ir::CType::get_usize(ctx->irCtx), false);
      }
    } else {
      ctx->Error("Invalid member name for pointer datatype " + ctx->color(instType->to_string()), fileRange);
    }
  } else {
    ctx->Error("Member access for expression of type " + ctx->color(instType->to_string()) + " is not supported",
               fileRange);
  }
  return nullptr;
  // NOLINTEND(readability-magic-numbers, clang-analyzer-core.CallAndMessage)
}

Json MemberAccess::to_json() const {
  return Json()
      ._("nodeType", "memberVariable")
      ._("instance", instance->to_json())
      ._("member", name)
      ._("fileRange", fileRange);
}

} // namespace qat::ast
