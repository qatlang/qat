#include "./to_conversion.hpp"
#include "../../IR/control_flow.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/c_type.hpp"
#include "../../IR/types/string_slice.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

ir::Value* ToConversion::emit(EmitCtx* ctx) {
  auto* val    = source->emit(ctx);
  auto* destTy = destinationType->emit(ctx);
  if (val->get_ir_type()->is_same(destTy)) {
    if (val->get_ir_type()->is_type_definition() || destTy->is_type_definition()) {
      return ir::Value::get(val->get_llvm(), destTy, false);
    } else {
      ctx->irCtx->Warning("Unnecessary conversion here. The expression is already of type " +
                              ctx->irCtx->highlightWarning(destTy->to_string()) +
                              ". Please remove this to avoid clutter.",
                          fileRange);
      return val;
    }
  } else {
    auto* typ     = val->get_ir_type();
    auto  loadRef = [&] {
      if (val->is_reference()) {
        val->load_ghost_pointer(ctx->irCtx->builder);
        val = ir::Value::get(ctx->irCtx->builder.CreateLoad(
                                 val->get_ir_type()->as_reference()->get_subtype()->get_llvm_type(), val->get_llvm()),
                              val->get_ir_type()->as_reference()->get_subtype(), false);
        typ = val->get_ir_type();
      } else {
        val->load_ghost_pointer(ctx->irCtx->builder);
      }
    };
    auto valType = val->is_reference() ? val->get_ir_type()->as_reference()->get_subtype() : val->get_ir_type();
    if (valType->is_ctype()) {
      valType = valType->as_ctype()->get_subtype();
    }
    if (valType->is_pointer()) {
      if (destTy->is_pointer() || (destTy->is_ctype() && destTy->as_ctype()->is_c_ptr())) {
        loadRef();
        auto targetTy = destTy->is_ctype() ? destTy->as_ctype()->get_subtype()->as_pointer() : destTy->as_pointer();
        if (!valType->as_pointer()->getOwner().is_same(targetTy->getOwner()) && !targetTy->getOwner().isAnonymous()) {
          ctx->Error(
              "This change of ownership of the pointer type is not allowed. Pointers with valid ownership can only be converted to anonymous ownership",
              fileRange);
        }
        if (valType->as_pointer()->isNullable() != targetTy->as_pointer()->isNullable()) {
          if (valType->as_pointer()->isNullable()) {
            auto  fun           = ctx->get_fn();
            auto* currBlock     = fun->get_block();
            auto* nullTrueBlock = new ir::Block(fun, currBlock);
            auto* restBlock     = new ir::Block(fun, currBlock->get_parent());
            restBlock->link_previous_block(currBlock);
            ctx->irCtx->builder.CreateCondBr(
                ctx->irCtx->builder.CreateICmpEQ(
                    ctx->irCtx->builder.CreatePtrDiff(
                        valType->as_pointer()->get_subtype()->is_type_sized()
                            ? valType->as_pointer()->get_subtype()->get_llvm_type()
                            : llvm::Type::getInt8Ty(ctx->irCtx->llctx),
                        valType->as_pointer()->isMulti() ? ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u})
                                                         : val->get_llvm(),
                        llvm::ConstantPointerNull::get(
                            llvm::PointerType::get(valType->as_pointer()->get_subtype()->is_type_sized()
                                                       ? valType->as_pointer()->get_subtype()->get_llvm_type()
                                                       : llvm::Type::getInt8Ty(ctx->irCtx->llctx),
                                                   ctx->irCtx->dataLayout.value().getProgramAddressSpace()))),
                    llvm::ConstantInt::get(ir::CType::get_ptrdiff(ctx->irCtx)->get_llvm_type(), 0u, true)),
                nullTrueBlock->get_bb(), restBlock->get_bb());
            nullTrueBlock->set_active(ctx->irCtx->builder);
            ir::Logic::panic_in_function(
                fun,
                {ir::StringSliceType::create_value(
                    ctx->irCtx,
                    "This is a null-pointer and hence cannot be converted to the non-nullable pointer type " +
                        destTy->to_string())},
                {}, fileRange, ctx);
            (void)ir::add_branch(ctx->irCtx->builder, restBlock->get_bb());
            restBlock->set_active(ctx->irCtx->builder);
          }
        }
        if (!valType->as_pointer()->get_subtype()->is_same(targetTy->get_subtype())) {
          ctx->Error(
              "The value to be converted is of type " + ctx->color(typ->to_string()) + " but the destination type is " +
                  ctx->color(destTy->to_string()) +
                  ". The subtype of the pointer types do not match and conversion between them is not allowed. Use casting instead",
              fileRange);
        }
        if (valType->as_pointer()->isMulti() != targetTy->as_pointer()->isMulti()) {
          if (valType->as_pointer()->isMulti()) {
            return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}), destTy, false);
          } else {
            auto newVal = ctx->get_fn()->get_block()->new_value(utils::unique_id(), destTy, false, fileRange);
            ctx->irCtx->builder.CreateStore(
                val->get_llvm(), ctx->irCtx->builder.CreateStructGEP(destTy->get_llvm_type(), newVal->get_llvm(), 0u));
            ctx->irCtx->builder.CreateStore(
                llvm::ConstantInt::get(ir::CType::get_usize(ctx->irCtx)->get_llvm_type(), 1u),
                ctx->irCtx->builder.CreateStructGEP(destTy->get_llvm_type(), newVal->get_llvm(), 1u));
            return newVal->to_new_ir_value();
          }
        }
        if (valType->as_pointer()->isMulti()) {
          val->get_llvm()->mutateType(destTy->get_llvm_type());
          return ir::Value::get(val->get_llvm(), destTy, false);
        } else {
          return ir::Value::get(ctx->irCtx->builder.CreatePointerCast(val->get_llvm(), destTy->get_llvm_type()), destTy,
                                false);
        }
      } else if (destTy->is_ctype() && (destTy->as_ctype()->is_intptr() || destTy->as_ctype()->is_intptr_unsigned())) {
        loadRef();
        return ir::Value::get(
            ctx->irCtx->builder.CreateBitCast(valType->as_pointer()->isMulti()
                                                  ? ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u})
                                                  : val->get_llvm(),
                                              destTy->get_llvm_type()),
            destTy, false);
      } else {
        ctx->Error("Pointer conversion to type " + ctx->color(destTy->to_string()) + " is not supported", fileRange);
      }
    } else if (valType->is_string_slice()) {
      auto destValTy = destTy->is_ctype() ? destTy->as_ctype()->get_subtype() : destTy;
      if (destTy->is_ctype() && destTy->as_ctype()->is_cstring()) {
        if (val->is_prerun_value()) {
          return ir::PrerunValue::get(val->get_llvm_constant()->getAggregateElement(0u), destTy);
        } else {
          loadRef();
          return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}), destTy, false);
        }
      } else if (destValTy->is_pointer() &&
                 (destValTy->as_pointer()->get_subtype()->is_unsigned_integer() ||
                  (destValTy->as_pointer()->get_subtype()->is_ctype() &&
                   destValTy->as_pointer()->get_subtype()->as_ctype()->get_subtype()->is_unsigned_integer())) &&
                 destValTy->as_pointer()->getOwner().isAnonymous() &&
                 (destValTy->as_pointer()->get_subtype()->is_unsigned_integer()
                      ? (destValTy->as_pointer()->get_subtype()->as_unsigned_integer()->getBitwidth() == 8u)
                      : (destValTy->as_pointer()
                             ->get_subtype()
                             ->as_ctype()
                             ->get_subtype()
                             ->as_unsigned_integer()
                             ->getBitwidth() == 8u)) &&
                 !destValTy->as_pointer()->isSubtypeVariable()) {
        loadRef();
        if (destValTy->as_pointer()->isMulti()) {
          val->get_llvm()->mutateType(destTy->get_llvm_type());
          return ir::Value::get(val->get_llvm(), destTy, false);
        } else {
          return ir::Value::get(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}), destTy, false);
        }
      } else {
        ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " + ctx->color(destTy->to_string()) +
                       " is not supported",
                   fileRange);
      }
    } else if (valType->is_integer()) {
      if (destTy->is_integer() || (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_integer())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true), destTy,
                              val->is_variable());
      } else if (destTy->is_unsigned_integer() ||
                 (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_unsigned_integer())) {
        loadRef();
        ctx->irCtx->Warning("Conversion from signed integer to unsigned integers can be lossy", fileRange);
        if (valType->as_integer()->get_bitwidth() == destTy->as_unsigned_integer()->getBitwidth()) {
          return ir::Value::get(ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true),
                                destTy, false);
        } else {
          return ir::Value::get(ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true),
                                destTy, false);
        }
      } else if (destTy->is_float() || (destTy->is_ctype() && destTy->as_ctype()->is_float())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateSIToFP(val->get_llvm(), destTy->get_llvm_type()), destTy,
                              false);
      } else {
        ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " + ctx->color(destTy->to_string()) +
                       " is not supported",
                   fileRange);
      }
    } else if (valType->is_unsigned_integer()) {
      if (destTy->is_unsigned_integer() ||
          (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_unsigned_integer())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), false),
                              destTy, false);
      } else if (destTy->is_integer() || (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_integer())) {
        loadRef();
        ctx->irCtx->Warning("Conversion from unsigned integer to signed integers can be lossy", fileRange);
        if (typ->as_unsigned_integer()->getBitwidth() == destTy->as_integer()->get_bitwidth()) {
          return ir::Value::get(val->get_llvm(), destTy, false);
        } else {
          return ir::Value::get(ctx->irCtx->builder.CreateIntCast(val->get_llvm(), destTy->get_llvm_type(), true),
                                destTy, false);
        }
      } else if (destTy->is_float() || (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_float())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateUIToFP(val->get_llvm(), destTy->get_llvm_type()), destTy,
                              false);
      } else {
        ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " + ctx->color(destTy->to_string()) +
                       " is not supported",
                   fileRange);
      }
    } else if (valType->is_float()) {
      if (destTy->is_float() || (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_float())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateFPCast(val->get_llvm(), destTy->get_llvm_type()), destTy,
                              false);
      } else if (destTy->is_integer() || (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_integer())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateFPToSI(val->get_llvm(), destTy->get_llvm_type()), destTy,
                              false);
      } else if (destTy->is_unsigned_integer() ||
                 (destTy->is_ctype() && destTy->as_ctype()->get_subtype()->is_unsigned_integer())) {
        loadRef();
        return ir::Value::get(ctx->irCtx->builder.CreateFPToUI(val->get_llvm(), destTy->get_llvm_type()), destTy,
                              false);
      } else {
        ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " + ctx->color(destTy->to_string()) +
                       " is not supported",
                   fileRange);
      }
    } else if (valType->is_choice()) {
      if ((destTy->is_underlying_type_integer() || destTy->is_underlying_type_unsigned()) &&
          valType->as_choice()->has_provided_type() &&
          (destTy->is_underlying_type_integer() ==
           valType->as_choice()->get_provided_type()->is_underlying_type_integer())) {
        if (destTy->is_underlying_type_integer()) {
          auto desIntTy  = destTy->get_underlying_integer_type();
          auto provIntTy = valType->as_choice()->get_provided_type()->get_underlying_integer_type();
          if (desIntTy->is_same(provIntTy)) {
            loadRef();
            return ir::Value::get(val->get_llvm(), destTy, false);
          } else {
            ctx->Error("The underlying type of the choice type " + ctx->color(valType->to_string()) + " is " +
                           ctx->color(valType->as_choice()->get_provided_type()->to_string()) +
                           ", but the type for conversion is " + ctx->color(destTy->to_string()),
                       fileRange);
          }
        } else {
          auto desIntTy  = destTy->get_underlying_unsigned_type();
          auto provIntTy = valType->as_choice()->get_provided_type()->get_underlying_unsigned_type();
          if (desIntTy->is_same(provIntTy)) {
            loadRef();
            return ir::Value::get(val->get_llvm(), destTy, false);
          } else {
            ctx->Error("The underlying type of the choice type " + ctx->color(valType->to_string()) + " is " +
                           ctx->color(valType->as_choice()->get_provided_type()->to_string()) +
                           ", but the type for conversion is " + ctx->color(destTy->to_string()),
                       fileRange);
          }
        }
      } else {
        ctx->Error("The underlying type of the choice type " + ctx->color(valType->to_string()) + " is " +
                       ctx->color(valType->as_choice()->get_provided_type()->to_string()) +
                       ", but the target type for conversion is " + ctx->color(destTy->to_string()),
                   fileRange);
      }
    } else {
      ctx->Error("Conversion from " + ctx->color(typ->to_string()) + " to " + ctx->color(destTy->to_string()) +
                     " is not supported",
                 fileRange);
    }
  }
}

Json ToConversion::to_json() const {
  return Json()
      ._("nodeType", "toConversion")
      ._("instance", source->to_json())
      ._("targetType", destinationType->to_json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast
