#include "./local_declaration.hpp"
#include "../../show.hpp"
#include "../constants/integer_literal.hpp"
#include "../constants/null_pointer.hpp"
#include "../constants/unsigned_literal.hpp"
#include "../expressions/array_literal.hpp"
#include "../expressions/constructor_call.hpp"
#include "../expressions/copy.hpp"
#include "../expressions/default.hpp"
#include "../expressions/mix_type_initialiser.hpp"
#include "../expressions/move.hpp"
#include "../expressions/plain_initialiser.hpp"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Casting.h"

namespace qat::ast {

LocalDeclaration::LocalDeclaration(QatType* _type, bool _isRef, bool _isPtr, String _name, Expression* _value,
                                   bool _variability, utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), type(_type), name(std::move(_name)), value(_value), variability(_variability),
      isRef(_isRef), isPtr(_isPtr){SHOW("Name for local declaration is " << name)}

                         IR::Value
                         * LocalDeclaration::emit(IR::Context * ctx) {
  auto* block = ctx->fn->getBlock();
  if (block->hasValue(name)) {
    ctx->Error("A local value named " + ctx->highlightError(name) +
                   " already exists in this scope. Please change name of this "
                   "declaration or check the logic",
               fileRange);
  } else if (block->hasAlias(name)) {
    ctx->Error("An alias named " + ctx->highlightError(name) +
                   " already exists in this scope. Please change name of this "
                   "declaration or check the logic",
               fileRange);
  }
  IR::QatType* declType = nullptr;

  // EDGE CASE -> The following code avoids multiple allocations for newly
  // created values, that are just meant to be assigned to the new entity
  if (value && (value->nodeType() == NodeType::arrayLiteral)) {
    auto* arr = (ast::ArrayLiteral*)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isArray()) {
        ctx->Error("The type for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the provided value is not compatible",
                   value->fileRange);
      }
      auto* loc  = block->newValue(name, declType, variability);
      arr->local = loc;
      (void)arr->emit(ctx);
      return nullptr;
    } else {
      arr->name  = name;
      arr->isVar = variability;
      (void)arr->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::plainInitialiser)) {
    auto* plain = (ast::PlainInitialiser*)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isCoreType()) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a core type. So the value provided cannot "
                       "be a plain initialiser",
                   value->fileRange);
      }
      if (!(plain->type)) {
        if (type->typeKind() == TypeKind::named) {
          plain->type = (NamedType*)type;
        } else {
          ctx->Error("Invalid type provided for plain initialisation", type->fileRange);
        }
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto* plainTy = plain->type->emit(ctx);
      if (declType->isSame(plainTy)) {
        auto* loc    = block->newValue(name, declType, variability);
        plain->local = loc;
        (void)plain->emit(ctx);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " + ctx->highlightError(plainTy->toString()),
                   value->fileRange);
      }
    } else {
      plain->irName = name;
      plain->isVar  = variability;
      (void)plain->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::constructorCall) &&
             !((((ast::ConstructorCall*)value))->isHeaped)) {
    auto* cons = (ast::ConstructorCall*)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isCoreType()) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a core type",
                   fileRange);
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto* constrTy = cons->type->emit(ctx);
      if (declType->isSame(constrTy)) {
        auto* loc   = block->newValue(name, declType, variability);
        cons->local = loc;
        (void)cons->emit(ctx);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " + ctx->highlightError(constrTy->toString()),
                   fileRange);
      }
    } else {
      cons->irName = name;
      cons->isVar  = variability;
      (void)cons->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::mixTypeInitialiser)) {
    auto* mixTyIn = (ast::MixTypeInitialiser*)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isCoreType()) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a core type",
                   fileRange);
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto* constrTy = mixTyIn->type->emit(ctx);
      if (declType->isSame(constrTy)) {
        auto* loc      = block->newValue(name, declType, variability);
        mixTyIn->local = loc;
        (void)mixTyIn->emit(ctx);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " + ctx->highlightError(constrTy->toString()),
                   fileRange);
      }
    } else {
      mixTyIn->irName = name;
      mixTyIn->isVar  = variability;
      (void)mixTyIn->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::Default)) {
    if (type) {
      declType     = type->emit(ctx);
      auto* defVal = (Default*)value;
      defVal->setType(declType);
      defVal->irName = name;
      defVal->isVar  = variability;
      (void)defVal->emit(ctx);
      return nullptr;
    } else {
      ctx->Error("No type provided for creating default value", fileRange);
    }
  } else if (value && (value->nodeType() == NodeType::moveExpression)) {
    SHOW("Value assigned to declaration is Move exp")
    auto* moveVal = (Move*)value;
    if (type) {
      declType       = type->emit(ctx);
      moveVal->local = ctx->fn->getBlock()->newValue(name, declType, variability);
      (void)moveVal->emit(ctx);
      return nullptr;
    } else if (!isRef) {
      moveVal->irName = name;
      moveVal->isVar  = variability;
      (void)moveVal->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::copyExpression)) {
    SHOW("Value assigned to declaration is Copy exp")
    auto* copyVal = (Copy*)value;
    if (type) {
      declType       = type->emit(ctx);
      copyVal->local = ctx->fn->getBlock()->newValue(name, declType, variability);
      (void)copyVal->emit(ctx);
      return nullptr;
    } else if (!isRef) {
      copyVal->irName = name;
      copyVal->isVar  = variability;
      (void)copyVal->emit(ctx);
      return nullptr;
    }
  }

  // EDGE CASE ends here

  IR::Value* expVal = nullptr;
  if (value) {
    if (value->nodeType() == NodeType::nullPointer) {
      if (type) {
        declType = type->emit(ctx);
        if (declType->isPointer() || (declType->isReference() && declType->asReference()->getSubType()->isPointer())) {
          auto* typeToSet =
              declType->isReference() ? declType->asReference()->getSubType()->asPointer() : declType->asPointer();
          ((NullPointer*)value)->setType(typeToSet->isSubtypeVariable(), typeToSet->getSubType());
        } else {
          ctx->Error("Invalid type recognised for the value to be assigned, "
                     "which is a null pointer. Expected a pointer type in the "
                     "declaration",
                     fileRange);
        }
      }
    } else if (value->nodeType() == NodeType::integerLiteral) {
      if (type) {
        declType = type->emit(ctx);
        if (!isRef) {
          if (declType->isInteger() || declType->isUnsignedInteger()) {
            ((IntegerLiteral*)value)->setType(declType);
          } else {
            ctx->Error("Invalid type to set for integer literal", fileRange);
          }
        } else {
          ctx->Error("The value to be assigned is a signed integer and hence this declaration cannot be a reference",
                     value->fileRange);
        }
      }
    } else if (value->nodeType() == NodeType::unsignedLiteral) {
      if (type) {
        declType = type->emit(ctx);
        if (!isRef) {
          if (declType->isInteger() || declType->isUnsignedInteger()) {
            ((UnsignedLiteral*)value)->setType(declType);
          } else {
            ctx->Error("Invalid type to set for unsigned integer literal", fileRange);
          }
        } else {
          ctx->Error("The value to be assigned is an unsigned integer and hence this declaration cannot be a reference",
                     value->fileRange);
        }
      }
    }
    expVal = value->emit(ctx);
    SHOW("Type of value to be assigned to local value " << name << " is " << expVal->getType()->toString())
  }
  if (type) {
    if (!declType) {
      declType = type->emit(ctx);
    }
    if (value && ((declType->isReference() && !expVal->isReference())
                      ? !declType->asReference()->getSubType()->isSame(expVal->getType())
                      : !declType->isSame(expVal->getType()))) {
      ctx->Error("Type of the local value " + ctx->highlightError(name) +
                     " does not match the expression to be assigned",
                 fileRange);
    }
  } else {
    SHOW("No type for decl. Getting type from value")
    if (expVal) {
      SHOW("Getting type from expression")
      declType = expVal->getType();
      if (expVal->getType()->isReference()) {
        if (!isRef) {
          declType = expVal->getType()->asReference()->getSubType();
        }
      } else if (declType->isFunction()) {
        if (!isPtr) {
          ctx->Error(
              "The value to be assigned is a pointer to a function, so please add a pointer hint to this declaration",
              fileRange);
        }
        declType       = IR::PointerType::get(false, declType, ctx->llctx);
        auto* fnCast   = ctx->builder.CreateBitCast(expVal->getLLVM(), declType->getLLVMType());
        auto* newValue = block->newValue(name, declType, variability);
        ctx->builder.CreateStore(fnCast, newValue->getLLVM());
        return nullptr;
      }
    } else {
      ctx->Error("Type inference for declarations require a value", fileRange);
    }
  }
  if (declType->isReference() && ((!expVal->getType()->isReference()) && expVal->isImplicitPointer())) {
    if (declType->asReference()->isSubtypeVariable() && (!expVal->isVariable())) {
      ctx->Error("The referred type of the left hand side has variability, but the "
                 "value provided for initialisation do not have variability",
                 value->fileRange);
    }
  } else if (declType->isReference() && expVal->getType()->isReference()) {
    if (declType->asReference()->isSubtypeVariable() && (!expVal->getType()->asReference()->isSubtypeVariable())) {
      ctx->Error("The reference on the left hand side refers to a value with "
                 "variability, but the value provided for initialisation is a "
                 "reference that refers to a value without variability",
                 value->fileRange);
    }
  }
  auto* newValue = block->newValue(name, declType, type ? type->isVariable() : variability);
  if (expVal) {
    if ((expVal->getType()->isCoreType() && expVal->isImplicitPointer()) ||
        (expVal->getType()->isReference() && expVal->getType()->asReference()->getSubType()->isCoreType())) {
      auto* cTy = expVal->isReference() ? expVal->getType()->asReference()->getSubType()->asCore()
                                        : expVal->getType()->asCore();
      if (cTy->hasCopyConstructor()) {
        if (expVal->isReference()) {
          expVal->loadImplicitPointer(ctx->builder);
        }
        auto* cpFn = cTy->getCopyConstructor();
        ctx->Warning("The copy constructor of core type " + ctx->highlightWarning(cTy->getFullName()) +
                         " is invoked here. Please make the copy explicit.",
                     value->fileRange);
        (void)cpFn->call(ctx, {newValue->getAlloca(), expVal->getLLVM()}, ctx->getMod());
        return nullptr;
      } else if (cTy->hasMoveConstructor()) {
        if (expVal->isReference()) {
          expVal->loadImplicitPointer(ctx->builder);
        }
        auto* mvFn = cTy->getMoveConstructor();
        ctx->Warning("The move constructor of core type " + ctx->highlightWarning(cTy->getFullName()) +
                         " is invoked here. Please make the move explicit.",
                     value->fileRange);
        (void)mvFn->call(ctx, {newValue->getAlloca(), expVal->getLLVM()}, ctx->getMod());
        return nullptr;
      }
    }
    SHOW("Creating store")
    if ((expVal->isImplicitPointer() && !declType->isReference()) ||
        (expVal->isImplicitPointer() && declType->isReference() && expVal->getType()->isReference())) {
      expVal->loadImplicitPointer(ctx->builder);
    } else if (!declType->isReference() && expVal->getType()->isReference()) {
      expVal = new IR::Value(ctx->builder.CreateLoad(declType->getLLVMType(), expVal->getLLVM()), declType, false,
                             IR::Nature::temporary);
    }
    ctx->builder.CreateStore(expVal->getLLVM(), newValue->getAlloca());
    SHOW("llvm::StoreInst created")
  }
  return nullptr;
}

Json LocalDeclaration::toJson() const {
  return Json()
      ._("nodeType", "localDeclaration")
      ._("name", name)
      ._("isVariable", variability)
      ._("hasType", (type != nullptr))
      ._("type", (type != nullptr) ? type->toJson() : Json())
      ._("value", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast