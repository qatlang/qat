#include "./local_declaration.hpp"
#include "../../IR/types/maybe.hpp"
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

LocalDeclaration::LocalDeclaration(QatType* _type, bool _isRef, Identifier _name, Maybe<Expression*> _value,
                                   bool _variability, FileRange _fileRange)
    : Sentence(std::move(_fileRange)), type(_type), name(std::move(_name)), value(_value), variability(_variability),
      isRef(_isRef){SHOW("Name for local declaration is " << name.value)}

          IR::Value
          * LocalDeclaration::emit(IR::Context * ctx) {
  auto* block = ctx->getActiveFunction()->getBlock();
  if (block->hasValue(name.value)) {
    ctx->Error("A local value named " + ctx->highlightError(name.value) +
                   " already exists in this scope. Please change name of this "
                   "declaration or check the logic",
               fileRange);
  } else {
    ctx->genericNameCheck(name.value, name.range);
  }
  IR::QatType* declType = nullptr;
  if (type) {
    SHOW("Type for local declaration: " << type->toString());
  }

  auto maybeTypeCheck = [&]() {
    if (declType && declType->isMaybe() && !variability) {
      ctx->Warning("The type of the declaration is " + ctx->highlightWarning(declType->toString()) +
                       ", but the local declaration is not a variable. And hence, it might not be usable",
                   fileRange);
    }
  };
  auto maybeTagFill = [&](IR::Value* loc) {
    if (declType && declType->isMaybe()) {
      ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
                               declType->asMaybe()->hasSizedSubType(ctx)
                                   ? ctx->builder.CreateStructGEP(loc->getType()->getLLVMType(), loc->getLLVM(), 0u)
                                   : loc->getLLVM());
    }
  };

  SHOW("Edge cases starts here")
  // EDGE CASE -> The following code avoids multiple allocations for newly
  // created values, that are just meant to be assigned to the new entity
  if (value.has_value() && (value.value()->nodeType() == NodeType::arrayLiteral)) {
    auto* arr = (ast::ArrayLiteral*)(value.value());
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      if (!declType->isArray() && !(declType->isMaybe() && declType->asMaybe()->getSubType()->isArray())) {
        ctx->Error("The type for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the provided value is not compatible",
                   value.value()->fileRange);
      }
      auto* loc  = block->newValue(name.value, declType, variability, name.range);
      arr->local = loc;
      (void)arr->emit(ctx);
      maybeTagFill(loc);
      return nullptr;
    } else {
      arr->name  = name;
      arr->isVar = variability;
      (void)arr->emit(ctx);
      return nullptr;
    }
  } else if (value.has_value() && (value.value()->nodeType() == NodeType::plainInitialiser)) {
    auto* plain = (ast::PlainInitialiser*)(value.value());
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      if (!declType->isCoreType() && (declType->isMaybe() && !declType->asMaybe()->getSubType()->isCoreType())) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a core type. So the value provided cannot "
                       "be a plain initialiser",
                   value.value()->fileRange);
      }
      if (!(plain->type)) {
        plain->type = type;
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto* plainTy = plain->type->emit(ctx);
      if (declType->isSame(plainTy) || (declType->isMaybe() && declType->asMaybe()->getSubType()->isSame(plainTy))) {
        auto* loc    = block->newValue(name.value, declType, variability, name.range);
        plain->local = loc;
        (void)plain->emit(ctx);
        maybeTagFill(loc);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " + ctx->highlightError(plainTy->toString()),
                   value.value()->fileRange);
      }
    } else {
      plain->irName = name;
      plain->isVar  = variability;
      (void)plain->emit(ctx);
      return nullptr;
    }
  } else if (value.has_value() && (value.value()->nodeType() == NodeType::constructorCall)) {
    auto* cons  = (ast::ConstructorCall*)(value.value());
    bool  isOwn = (((ast::ConstructorCall*)(value.value())))->isOwning();
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      if (!isOwn && !declType->isCoreType() &&
          (declType->isMaybe() && !declType->asMaybe()->getSubType()->isCoreType())) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a core type",
                   fileRange);
      } else if (isOwn &&
                 (!declType->isPointer() ||
                  (declType->isPointer() && !declType->asPointer()->getSubType()->isCoreType())) &&
                 ((declType->isMaybe() && !declType->asMaybe()->getSubType()->isPointer()) ||
                  (declType->isMaybe() && declType->asMaybe()->getSubType()->isPointer() &&
                   !declType->asMaybe()->getSubType()->asPointer()->getSubType()->isCoreType()))) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a pointer",
                   fileRange);
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto* constrTy = cons->type->emit(ctx);
      if (isOwn ? (declType->isMaybe() ? declType->asMaybe()->getSubType()->asPointer()->isSame(constrTy)
                                       : declType->asPointer()->getSubType()->isSame(constrTy))
                : (declType->isMaybe() ? declType->asMaybe()->getSubType()->isSame(constrTy)
                                       : declType->isSame(constrTy))) {
        SHOW("Local Declaration => name : " << name.value << " type: " << declType->toString()
                                            << " variability: " << variability)
        auto* loc   = block->newValue(name.value, declType, variability, name.range);
        cons->local = loc;
        if (type) {
          cons->isVar = variability;
        }
        (void)cons->emit(ctx);
        maybeTagFill(loc);
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
  } else if (value.has_value() && (value.value()->nodeType() == NodeType::mixTypeInitialiser)) {
    auto* mixTyIn = (ast::MixTypeInitialiser*)(value.value());
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      if (!declType->isMix() && (declType->isMaybe() && !declType->asMaybe()->getSubType()->isMix())) {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       " and is not a mix type",
                   fileRange);
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto* mixTy = mixTyIn->type->emit(ctx);
      if (declType->isSame(mixTy) || (declType->isMaybe() && declType->asMaybe()->getSubType()->isSame(mixTy))) {
        auto* loc      = block->newValue(name.value, declType, variability, name.range);
        mixTyIn->local = loc;
        (void)mixTyIn->emit(ctx);
        maybeTagFill(loc);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " + ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " + ctx->highlightError(mixTy->toString()),
                   fileRange);
      }
    } else {
      mixTyIn->irName = name;
      mixTyIn->isVar  = variability;
      (void)mixTyIn->emit(ctx);
      return nullptr;
    }
  } else if (value && (value.value()->nodeType() == NodeType::Default)) {
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      auto* defVal = (Default*)(value.value());
      if (defVal->inferredType) {
        if (!defVal->inferredType.value()->isSame(declType)) {
          ctx->Error("Type of the declaration is " + ctx->highlightError(declType->toString()) +
                         " does not match the type provided for the " + ctx->highlightError("default") +
                         " expression, which is " + ctx->highlightError(defVal->inferredType.value()->toString()),
                     defVal->fileRange);
        }
      }
      defVal->setInferenceType(declType);
      defVal->irName = name;
      defVal->isVar  = variability;
      (void)defVal->emit(ctx);
      return nullptr;
    } else if (value.value()->isTypeInferred()) {
      auto* defVal   = ((Default*)value.value());
      defVal->irName = name;
      defVal->isVar  = variability;
      (void)defVal->emit(ctx);
      return nullptr;
    } else {
      ctx->Error("No type provided for creating default value", fileRange);
    }
  } else if (value && (value.value()->nodeType() == NodeType::moveExpression)) {
    SHOW("Value assigned to declaration is Move exp")
    auto* moveVal = (Move*)(value.value());
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      moveVal->local = ctx->getActiveFunction()->getBlock()->newValue(name.value, declType, variability, name.range);
      (void)moveVal->emit(ctx);
      return nullptr;
    } else if (!isRef) {
      moveVal->irName = name;
      moveVal->isVar  = variability;
      (void)moveVal->emit(ctx);
      return nullptr;
    }
  } else if (value && (value.value()->nodeType() == NodeType::copyExpression)) {
    SHOW("Value assigned to declaration is Copy exp")
    auto* copyVal = (Copy*)(value.value());
    if (type) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      copyVal->local = ctx->getActiveFunction()->getBlock()->newValue(name.value, declType, variability, name.range);
      (void)copyVal->emit(ctx);
      return nullptr;
    } else if (!isRef) {
      copyVal->irName = name;
      copyVal->isVar  = variability;
      (void)copyVal->emit(ctx);
      return nullptr;
    }
  }

  SHOW("Edge case ends here")

  IR::Value* expVal = nullptr;
  if (value.has_value()) {
    if (value.value()->nodeType() == NodeType::nullPointer) {
      if (type) {
        declType = type->emit(ctx);
        maybeTypeCheck();
        if (declType->isMaybe() && declType->asMaybe()->getSubType()->isPointer()) {
          value.value()->setInferenceType(declType->asMaybe()->getSubType()->asPointer());
        } else if (declType->isPointer() ||
                   (declType->isReference() && declType->asReference()->getSubType()->isPointer())) {
          auto* typeToSet =
              declType->isReference() ? declType->asReference()->getSubType()->asPointer() : declType->asPointer();
          value.value()->setInferenceType(typeToSet);
        } else {
          ctx->Error("Invalid type recognised for the value to be assigned, "
                     "which is a null pointer. Expected a pointer type in the "
                     "declaration",
                     fileRange);
        }
      }
    } else if (value.value()->nodeType() == NodeType::integerLiteral) {
      if (type) {
        declType = type->emit(ctx);
        maybeTypeCheck();
        if (!isRef) {
          if (declType->isMaybe() && (declType->asMaybe()->getSubType()->isInteger() ||
                                      declType->asMaybe()->getSubType()->isUnsignedInteger())) {
            value.value()->setInferenceType(declType->asMaybe()->getSubType());
          } else if (declType->isInteger() || declType->isUnsignedInteger()) {
            value.value()->setInferenceType(declType);
          } else {
            ctx->Error("Invalid type to set for integer literal", fileRange);
          }
        } else {
          ctx->Error("The value to be assigned is a signed integer and hence this declaration cannot be a reference",
                     value.value()->fileRange);
        }
      }
    } else if (value.value()->nodeType() == NodeType::unsignedLiteral) {
      if (type) {
        declType = type->emit(ctx);
        maybeTypeCheck();
        if (!isRef) {
          if (declType->isMaybe() && (declType->asMaybe()->getSubType()->isInteger() ||
                                      declType->asMaybe()->getSubType()->isUnsignedInteger())) {
            value.value()->setInferenceType(declType->asMaybe()->getSubType());
          } else if (declType->isInteger() || declType->isUnsignedInteger()) {
            value.value()->setInferenceType(declType);
          } else {
            ctx->Error("Invalid type to set for unsigned integer literal", fileRange);
          }
        } else {
          ctx->Error("The value to be assigned is an unsigned integer and hence this declaration cannot be a reference",
                     value.value()->fileRange);
        }
      }
    }
    SHOW("Emitting value")
    expVal = value.value()->emit(ctx);
    SHOW("Type of value to be assigned to local value " << name.value << " is " << expVal->getType()->toString())
  }
  SHOW("Type inference for value is complete")
  if (type) {
    SHOW("Checking & setting declType")
    if (!declType) {
      declType = type->emit(ctx);
      maybeTypeCheck();
    }
    SHOW("About to type match")
    if (value && (((declType->isReference() && !expVal->isReference()) &&
                   !declType->asReference()->getSubType()->isSame(expVal->getType())) &&
                  (declType->isMaybe() && !(declType->asMaybe()->getSubType()->isSame(expVal->getType()))) &&
                  !declType->isSame(expVal->getType()))) {
      ctx->Error("Type of the local value " + ctx->highlightError(name.value) + " is " +
                     ctx->highlightError(declType->toString()) +
                     " does not match the expression to be assigned which is of type " +
                     ctx->highlightError(expVal->getType()->toString()),
                 fileRange);
    }
  } else {
    SHOW("No type for decl. Getting type from value")
    if (expVal) {
      SHOW("Getting type from expression")
      declType = expVal->getType();
      maybeTypeCheck();
      if (expVal->getType()->isReference()) {
        if (!isRef) {
          declType = expVal->getType()->asReference()->getSubType();
        }
      } else if (declType->isFunction()) {
        declType       = IR::PointerType::get(false, declType, IR::PointerOwner::OfAnonymous(), false, ctx);
        auto* fnCast   = ctx->builder.CreateBitCast(expVal->getLLVM(), declType->getLLVMType());
        auto* newValue = block->newValue(name.value, declType, variability, name.range);
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
                 value.value()->fileRange);
    }
  } else if (declType->isReference() && expVal->getType()->isReference()) {
    if (declType->asReference()->isSubtypeVariable() && (!expVal->getType()->asReference()->isSubtypeVariable())) {
      ctx->Error("The reference on the left hand side refers to a value with "
                 "variability, but the value provided for initialisation is a "
                 "reference that refers to a value without variability",
                 value.value()->fileRange);
    }
  }
  SHOW("Creating new value")
  auto* newValue = block->newValue(name.value, declType, variability, name.range);
  if (expVal) {
    if (declType->isMaybe()) {
      SHOW("Got sub type")
      auto* subTy = declType->asMaybe()->getSubType();
      SHOW("Checking types and matching")
      if (expVal->getType()->isSame(subTy) ||
          (expVal->getType()->isReference() && expVal->getType()->asReference()->getSubType()->isSame(subTy))) {
        SHOW("Maybe : Subtype matches")
        if (expVal->isReference()) {
          expVal->loadImplicitPointer(ctx->builder);
          SHOW("Loaded implicit pointer for reference")
        }
        SHOW("Maybe: Checking Copy & Move constructor")
        if (subTy->isCoreType()) {
          SHOW("Has copy: " << subTy->asCore()->hasCopyConstructor())
          SHOW("Has move: " << subTy->asCore()->hasMoveConstructor())
        }
        if ((expVal->isImplicitPointer() || expVal->isReference()) && subTy->isCoreType() &&
            (subTy->asCore()->hasCopyConstructor() || subTy->asCore()->hasMoveConstructor())) {
          SHOW("Getting copy / move constructor")
          auto* constrFn = subTy->asCore()->hasCopyConstructor() ? subTy->asCore()->getCopyConstructor()
                                                                 : subTy->asCore()->getMoveConstructor();
          SHOW("Got copy/move constructor")
          auto* newRef = ctx->builder.CreateStructGEP(newValue->getType()->getLLVMType(), newValue->getAlloca(), 1u);
          (void)constrFn->call(ctx, {newRef, expVal->getLLVM()}, ctx->getMod());
          if (subTy->asCore()->hasCopyConstructor()) {
            ctx->Warning("The copy constructor of core type " + ctx->highlightWarning(subTy->asCore()->getFullName()) +
                             " is invoked here. Please make the copy explicit.",
                         value.value()->fileRange);
          } else {
            ctx->Warning("The move constructor of core type " + ctx->highlightWarning(subTy->asCore()->getFullName()) +
                             " is invoked here. Please make the move explicit.",
                         value.value()->fileRange);
          }
          SHOW("Called the constructor function")
          return nullptr;
        } else {
          expVal->loadImplicitPointer(ctx->builder);
          if (!expVal->getType()->isSame(subTy)) {
            expVal = new IR::Value(ctx->builder.CreateLoad(declType->getLLVMType(), expVal->getLLVM()), declType, false,
                                   IR::Nature::temporary);
          }
          // FIXME - Copy & move
          ctx->builder.CreateStore(expVal->getLLVM(),
                                   ctx->builder.CreateStructGEP(declType->getLLVMType(), newValue->getLLVM(), 1u));
        }
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 1u),
                                 ctx->builder.CreateStructGEP(declType->getLLVMType(), newValue->getLLVM(), 0u));
      } else if (expVal->getType()->isSame(declType) ||
                 (expVal->getType()->isReference() &&
                  expVal->getType()->asReference()->getSubType()->isSame(declType))) {
        SHOW("Maybe : Absolute type matches")
        if (!expVal->getType()->isSame(declType)) {
          expVal = new IR::Value(ctx->builder.CreateLoad(declType->getLLVMType(), expVal->getLLVM()),
                                 expVal->getType()->asReference()->getSubType(), false, IR::Nature::temporary);
        } else {
          expVal->loadImplicitPointer(ctx->builder);
        }
        ctx->builder.CreateStore(expVal->getLLVM(), newValue->getLLVM());
      } else {
        ctx->Error("Expected " + ctx->highlightError(declType->toString()) + " or " +
                       ctx->highlightError(declType->asMaybe()->getSubType()->toString()) +
                       " as the type of the value to be assigned",
                   fileRange);
      }
      return nullptr;
    } else if ((expVal->getType()->isCoreType() && expVal->isImplicitPointer()) ||
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
                     value.value()->fileRange);
        (void)cpFn->call(ctx, {newValue->getAlloca(), expVal->getLLVM()}, ctx->getMod());
        return nullptr;
      } else if (cTy->hasMoveConstructor()) {
        if (expVal->isReference()) {
          expVal->loadImplicitPointer(ctx->builder);
        }
        auto* mvFn = cTy->getMoveConstructor();
        ctx->Warning("The move constructor of core type " + ctx->highlightWarning(cTy->getFullName()) +
                         " is invoked here. Please make the move explicit.",
                     value.value()->fileRange);
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
  } else {
    if (declType && declType->isMaybe()) {
      if (declType->asMaybe()->hasSizedSubType(ctx)) {
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u),
                                 ctx->builder.CreateStructGEP(declType->getLLVMType(), newValue->getAlloca(), 0u));
        ctx->builder.CreateStore(llvm::Constant::getNullValue(declType->asMaybe()->getSubType()->getLLVMType()),
                                 ctx->builder.CreateStructGEP(declType->getLLVMType(), newValue->getAlloca(), 1u));
      } else {
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx->llctx), 0u), newValue->getAlloca(),
                                 0u);
      }
    } else {
      ctx->Error("Expected an expression to be initialised for the declaration. Only declaration with " +
                     ctx->highlightError("maybe") + " type can omit initialisation",
                 fileRange);
    }
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
      ._("hasValue", value.has_value())
      ._("value", value.has_value() ? value.value()->toJson() : Json())
      ._("fileRange", fileRange);
}

} // namespace qat::ast