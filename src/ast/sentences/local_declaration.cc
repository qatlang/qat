#include "./local_declaration.hpp"
#include "../../IR/types/maybe.hpp"
#include "../../show.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

ir::Value* LocalDeclaration::emit(EmitCtx* ctx) {
  auto* block = ctx->get_fn()->get_block();
  if (block->hasValue(name.value)) {
    ctx->Error("A local value named " + ctx->color(name.value) +
                   " already exists in this scope. Please change name of this "
                   "declaration or check the logic",
               fileRange);
  } else {
    ctx->genericNameCheck(name.value, name.range);
  }
  ir::Type* declType = nullptr;

  SHOW("Type for local declaration is " << (type ? type->to_string() : "not provided"));

  auto typeCheck = [&]() {
    if (declType && declType->is_maybe() && !variability) {
      ctx->irCtx->Warning("The type of the declaration is " + ctx->irCtx->highlightWarning(declType->to_string()) +
                              ", but the local declaration is not a variable. And hence, it might not be usable",
                          fileRange);
    }
    if (declType && !declType->is_type_sized()) {
      ctx->Error("The type " + ctx->color(declType->to_string()) + " is not sized and hence cannot be allocated",
                 fileRange);
    }
  };

  ir::Value* expVal = nullptr;
  if (value.has_value()) {
    if (type && value.value()->has_type_inferrance()) {
      declType = type->emit(ctx);
      typeCheck();
      value.value()->as_type_inferrable()->set_inference_type(declType);
    }
    if (value.value()->isLocalDeclCompatible()) {
      if ((type || declType)) {
        if (!declType) {
          declType = type->emit(ctx);
          typeCheck();
        }
        value.value()->asLocalDeclCompatible()->setLocalValue(
            ctx->get_fn()->get_block()->new_value(name.value, declType, variability, name.range));
      } else {
        auto* localDeclCompat   = value.value()->asLocalDeclCompatible();
        localDeclCompat->irName = name;
        localDeclCompat->isVar  = variability;
      }
      (void)value.value()->emit(ctx);
      return nullptr;
    }
    SHOW("Emitting value")
    expVal = value.value()->emit(ctx);
    SHOW("Type of value to be assigned to local value " << name.value << " is " << expVal->get_ir_type()->to_string())
  } else {
    if (type) {
      declType = type->emit(ctx);
      if (declType->is_trivially_movable()) {
        auto result = ctx->get_fn()->get_block()->new_value(name.value, declType, variability, name.range);
        ctx->irCtx->builder.CreateStore(llvm::Constant::getNullValue(declType->get_llvm_type()), result->get_llvm());
        return result->to_new_ir_value();
      } else {
        ctx->Error(
            "The type of the local declaration is " + ctx->color(declType->to_string()) +
                " which is not trivially movable. Expression to be assigned can only be skipped if the value is trivially movable",
            fileRange);
      }
    }
  }
  SHOW("Type inference for value is complete")
  if (type) {
    SHOW("Checking & setting declType")
    if (!declType) {
      declType = type->emit(ctx);
      typeCheck();
    }
    SHOW("About to type match")
    if (value && (((declType->is_reference() && !expVal->is_reference()) &&
                   !declType->as_reference()->get_subtype()->is_same(expVal->get_ir_type())) &&
                  (declType->is_maybe() && !(declType->as_maybe()->get_subtype()->is_same(expVal->get_ir_type()))) &&
                  !declType->is_same(expVal->get_ir_type()))) {
      ctx->Error("Type of the local value " + ctx->color(name.value) + " is " + ctx->color(declType->to_string()) +
                     " which is not compatible with the expression to be assigned which is of type " +
                     ctx->color(expVal->get_ir_type()->to_string()),
                 fileRange);
    }
  } else {
    SHOW("No type for decl. Getting type from value")
    if (expVal) {
      SHOW("Getting type from expression")
      declType = expVal->get_ir_type();
      typeCheck();
      if (expVal->get_ir_type()->is_reference()) {
        if (!isRef) {
          declType = expVal->get_ir_type()->as_reference()->get_subtype();
        }
      }
    } else {
      ctx->Error("Type inference for declarations require a value", fileRange);
    }
  }
  if (declType->is_reference() && ((!expVal->get_ir_type()->is_reference()) && expVal->is_ghost_pointer())) {
    if (declType->as_reference()->isSubtypeVariable() && (!expVal->is_variable())) {
      ctx->Error("The referred type of the left hand side has variability, but the "
                 "value provided for initialisation do not have variability",
                 value.value()->fileRange);
    }
  } else if (declType->is_reference() && expVal->get_ir_type()->is_reference()) {
    if (declType->as_reference()->isSubtypeVariable() &&
        (!expVal->get_ir_type()->as_reference()->isSubtypeVariable())) {
      ctx->Error("The reference on the left hand side refers to a value with "
                 "variability, but the value provided for initialisation is a "
                 "reference that refers to a value without variability",
                 value.value()->fileRange);
    }
  }
  SHOW("Creating new value")
  auto* new_value = block->new_value(name.value, declType, variability, name.range);
  if (expVal) {
    if (expVal->get_ir_type()->is_reference() || expVal->is_ghost_pointer()) {
      if (expVal->get_ir_type()->is_reference()) {
        expVal->load_ghost_pointer(ctx->irCtx->builder);
      }
      auto* expValTy = expVal->get_ir_type()->is_reference() ? expVal->get_ir_type()->as_reference()->get_subtype()
                                                             : expVal->get_ir_type();
      if (!expValTy->is_same(new_value->get_ir_type())) {
        ctx->Error("Type of the provided expression is " + ctx->color(expValTy->to_string()) +
                       " and does not match the type of the declaration which is " + ctx->color(declType->to_string()),
                   value.value()->fileRange);
      }
      if (expValTy->is_trivially_copyable() || expValTy->is_trivially_movable()) {
        ctx->irCtx->builder.CreateStore(ctx->irCtx->builder.CreateLoad(expValTy->get_llvm_type(), expVal->get_llvm()),
                                        new_value->get_llvm());
        if (!expValTy->is_trivially_copyable()) {
          if (expVal->is_reference() && !expVal->get_ir_type()->as_reference()->isSubtypeVariable()) {
            ctx->Error("This expression is of type " + ctx->color(expVal->get_ir_type()->to_string()) +
                           " which is a reference without variability and hence cannot be trivially moved from",
                       value.value()->fileRange);
          } else if (!expVal->is_variable()) {
            ctx->Error("This expression does not have variability and hence cannot be trivially moved from",
                       value.value()->fileRange);
          }
          // MOVE WARNING
          ctx->irCtx->Warning("There is a trivial move occuring here. Do you want to use " + ctx->color("'move") +
                                  " to make it explicit and clear?",
                              value.value()->fileRange);
          ctx->irCtx->builder.CreateStore(llvm::ConstantExpr::getNullValue(expValTy->get_llvm_type()),
                                          expVal->get_llvm());
          if (expVal->is_local_value()) {
            ctx->get_fn()->get_block()->addMovedValue(expVal->get_local_id().value());
          }
        }
      } else {
        // NON-TRIVIAL COPY & MOVE ERROR
        ctx->Error("The expression provided is of type " + ctx->color(expValTy->to_string()) +
                       " which cannot be trivially copied or moved. Please do " + ctx->color("'copy") + " or " +
                       ctx->color("'move") + " accordingly",
                   value.value()->fileRange);
      }
    } else {
      if (!expVal->get_ir_type()->is_same(new_value->get_ir_type())) {
        ctx->Error("Type of the provided expression is " + ctx->color(expVal->get_ir_type()->to_string()) +
                       " and does not match the type of the declaration which is " + ctx->color(declType->to_string()),
                   value.value()->fileRange);
      }
      ctx->irCtx->builder.CreateStore(expVal->get_llvm(), new_value->getAlloca());
    }
    return nullptr;
  } else {
    if (declType && declType->is_maybe()) {
      if (declType->as_maybe()->has_sized_sub_type(ctx->irCtx)) {
        ctx->irCtx->builder.CreateStore(
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u),
            ctx->irCtx->builder.CreateStructGEP(declType->get_llvm_type(), new_value->getAlloca(), 0u));
        ctx->irCtx->builder.CreateStore(
            llvm::Constant::getNullValue(declType->as_maybe()->get_subtype()->get_llvm_type()),
            ctx->irCtx->builder.CreateStructGEP(declType->get_llvm_type(), new_value->getAlloca(), 1u));
      } else {
        ctx->irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->irCtx->llctx), 0u),
                                        new_value->getAlloca(), 0u);
      }
    } else {
      ctx->Error("Expected an expression to be initialised for the declaration. Only declaration with " +
                     ctx->color("maybe") + " type can omit initialisation",
                 fileRange);
    }
  }
  return nullptr;
}

Json LocalDeclaration::to_json() const {
  return Json()
      ._("nodeType", "localDeclaration")
      ._("name", name)
      ._("is_variable", variability)
      ._("hasType", (type != nullptr))
      ._("type", (type != nullptr) ? type->to_json() : Json())
      ._("hasValue", value.has_value())
      ._("value", value.has_value() ? value.value()->to_json() : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast