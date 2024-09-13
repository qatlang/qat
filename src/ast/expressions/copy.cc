#include "./copy.hpp"
#include "../../IR/logic.hpp"
#include "../../IR/types/qat_type.hpp"

namespace qat::ast {

ir::Value* Copy::emit(EmitCtx* ctx) {
  FnAtEnd fnObj{[&] { createIn = nullptr; }};
  if (isExpSelf) {
    if (!ctx->get_fn()->is_method()) {
      ctx->Error("Cannot perform copy on the parent instance as this is not a member function", fileRange);
    } else {
      auto memFn = (ir::Method*)ctx->get_fn();
      if (memFn->is_static_method()) {
        ctx->Error("Cannot perform copy on the parent instance as this is a static function", fileRange);
      }
      if (memFn->isConstructor()) {
        ctx->Error("Cannot perform copy on the parent instance as this is a constructor", fileRange);
      }
    }
  } else {
    if (exp->nodeType() == NodeType::SELF) {
      ctx->Error("Do not use this syntax for copying the parent instance. Use " + ctx->color("''copy") + " instead",
                 fileRange);
    }
  }
  auto* expEmit = exp->emit(ctx);
  auto* expTy   = expEmit->get_ir_type();
  if (expEmit->is_ghost_reference() || expTy->is_reference()) {
    if (expTy->is_reference()) {
      expEmit->load_ghost_reference(ctx->irCtx->builder);
    }
    auto* candTy =
        expEmit->is_reference() ? expEmit->get_ir_type()->as_reference()->get_subtype() : expEmit->get_ir_type();
    if (!isAssignment) {
      if (candTy->is_copy_constructible()) {
        bool shouldLoadValue = false;
        if (isLocalDecl()) {
          if (!candTy->is_same(localValue->get_ir_type())) {
            ctx->Error("The type provided in the local declaration does not match the type of the value to be copied",
                       fileRange);
          }
          createIn = ir::Value::get(localValue->get_alloca(), ir::ReferenceType::get(true, candTy, ctx->irCtx), false);
        } else if (canCreateIn()) {
          if (createIn->is_reference() || createIn->is_ghost_reference()) {
            auto expTy = createIn->is_ghost_reference() ? createIn->get_ir_type()
                                                        : createIn->get_ir_type()->as_reference()->get_subtype();
            if (!expTy->is_same(candTy)) {
              ctx->Error("Trying to optimise the copy by creating in-place, but the expression type is " +
                             ctx->color(candTy->to_string()) +
                             " which does not match with the underlying type for in-place creation which is " +
                             ctx->color(expTy->to_string()),
                         fileRange);
            }
          } else {
            ctx->Error("Trying to optimise the copy by creating in-place, but the containing type is not a reference",
                       fileRange);
          }
        } else {
          if (irName.has_value()) {
            createIn = ctx->get_fn()->get_block()->new_value(irName->value, candTy, isVar, irName->range);
          } else {
            shouldLoadValue = true;
            createIn        = ir::Value::get(ir::Logic::newAlloca(ctx->get_fn(), None, candTy->get_llvm_type()),
                                             ir::ReferenceType::get(true, candTy, ctx->irCtx), false);
          }
        }
        (void)candTy->copy_construct_value(ctx->irCtx, createIn, expEmit, ctx->get_fn());
        if (shouldLoadValue) {
          return ir::Value::get(ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), createIn->get_llvm()), candTy,
                                true);
        } else {
          return createIn;
        }
      } else if (candTy->is_trivially_copyable()) {
        if (isLocalDecl()) {
          if (!candTy->is_same(localValue->get_ir_type())) {
            ctx->Error("The type provided in the local declaration does not match the type of the value to be copied",
                       fileRange);
          }
          createIn = ir::Value::get(localValue->get_alloca(), ir::ReferenceType::get(true, candTy, ctx->irCtx), false);
        }
        if (canCreateIn()) {
          if (!isLocalDecl()) {
            if (createIn->is_reference() || createIn->is_ghost_reference()) {
              auto expTy = createIn->is_ghost_reference() ? createIn->get_ir_type()
                                                          : createIn->get_ir_type()->as_reference()->get_subtype();
              if (!expTy->is_same(candTy)) {
                ctx->Error("Trying to optimise the copy by creating in-place, but the expression type is " +
                               ctx->color(candTy->to_string()) +
                               " which does not match with the underlying type for in-place creation which is " +
                               ctx->color(expTy->to_string()),
                           fileRange);
              }
            } else {
              ctx->Error("Trying to optimise the copy by creating in-place, but the containing type is not a reference",
                         fileRange);
            }
          }
          // FIXME - Use memcpy?
          ctx->irCtx->builder.CreateStore(ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), expEmit->get_llvm()),
                                          createIn->get_llvm());
          return createIn;
        } else {
          return ir::Value::get(ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), expEmit->get_llvm()), candTy,
                                true);
        }
      } else {
        ctx->Error((candTy->is_struct() ? "Core type " : (candTy->is_mix() ? "Mix type " : "Type ")) +
                       ctx->color(candTy->to_string()) +
                       " does not have a copy constructor and is also not trivially copyable",
                   fileRange);
      }
    } else {
      if (createIn->is_ghost_reference()
              ? createIn->get_ir_type()->is_same(candTy)
              : (createIn->is_reference() && createIn->get_ir_type()->as_reference()->get_subtype()->is_same(candTy))) {
        if (expEmit->is_reference()) {
          expEmit->load_ghost_reference(ctx->irCtx->builder);
        }
        if (candTy->is_trivially_copyable()) {
          ctx->irCtx->builder.CreateStore(ctx->irCtx->builder.CreateLoad(candTy->get_llvm_type(), expEmit->get_llvm()),
                                          createIn->get_llvm());
          return createIn;
        } else if (candTy->is_copy_assignable()) {
          (void)candTy->copy_assign_value(ctx->irCtx, createIn, expEmit, ctx->get_fn());
          return createIn;
        } else {
          ctx->Error((candTy->is_struct() ? "Core type " : (candTy->is_mix() ? "Mix type " : "Type ")) +
                         ctx->color(candTy->to_string()) +
                         " does not have a copy assignment operator and is also not trivially copyable",
                     fileRange);
        }
      } else {
        ctx->Error("Tried to optimise copy assignment by in-place copying. The left hand side is of type " +
                       ctx->color(createIn->get_ir_type()->to_string()) + " but the right hand side is of type " +
                       ctx->color(expEmit->get_ir_type()->to_string()) +
                       " and hence copy assignment cannot be performed",
                   fileRange);
      }
    }
  } else {
    ctx->Error(
        "This expression is a value. Expression provided should either be a reference, a local or a global variable. The provided value does not reside in an address and hence cannot be copied from",
        fileRange);
  }
  return nullptr;
}

Json Copy::to_json() const {
  return Json()._("nodeType", "copyExpression")._("expression", exp->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast
