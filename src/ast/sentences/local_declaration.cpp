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

  SHOW("Type for local declaration is " << (type ? type->toString() : "not provided"));

  auto maybeTypeCheck = [&]() {
    if (declType && declType->isMaybe() && !variability) {
      ctx->Warning("The type of the declaration is " + ctx->highlightWarning(declType->toString()) +
                       ", but the local declaration is not a variable. And hence, it might not be usable",
                   fileRange);
    }
  };

  IR::Value* expVal = nullptr;
  if (value.has_value()) {
    if (type && value.value()->hasTypeInferrance()) {
      declType = type->emit(ctx);
      maybeTypeCheck();
      value.value()->asTypeInferrable()->setInferenceType(declType);
    }
    if (value.value()->isLocalDeclCompatible()) {
      if ((type || declType)) {
        if (!declType) {
          declType = type->emit(ctx);
          maybeTypeCheck();
        }
        value.value()->asLocalDeclCompatible()->setLocalValue(
            ctx->getActiveFunction()->getBlock()->newValue(name.value, declType, variability, name.range));
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
    if (expVal->getType()->isReference() || expVal->isImplicitPointer()) {
      if (expVal->getType()->isReference()) {
        expVal->loadImplicitPointer(ctx->builder);
      }
      auto* expValTy =
          expVal->getType()->isReference() ? expVal->getType()->asReference()->getSubType() : expVal->getType();
      if (!expValTy->isSame(newValue->getType())) {
        ctx->Error("Type of the provided expression is " + ctx->highlightError(expValTy->toString()) +
                       " and does not match the type of the declaration which is " +
                       ctx->highlightError(declType->toString()),
                   value.value()->fileRange);
      }
      if (expValTy->isTriviallyCopyable() || expValTy->isTriviallyMovable()) {
        ctx->builder.CreateStore(ctx->builder.CreateLoad(expValTy->getLLVMType(), expVal->getLLVM()),
                                 newValue->getLLVM());
        if (!expValTy->isTriviallyCopyable()) {
          // MOVE WARNING
          ctx->Warning("There is a trivial move occuring here. Do you want to use " + ctx->highlightWarning("'move") +
                           " to make it explicit and clear?",
                       value.value()->fileRange);
          ctx->builder.CreateStore(llvm::ConstantExpr::getNullValue(expValTy->getLLVMType()), expVal->getLLVM());
          if (expVal->isLocalToFn()) {
            ctx->getActiveFunction()->getBlock()->addMovedValue(expVal->getLocalID());
          }
        }
      } else {
        // NON-TRIVIAL COPY & MOVE ERROR
        ctx->Error("The expression provided is of type " + ctx->highlightError(expValTy->toString()) + ". Type " +
                       ctx->highlightError(expValTy->toString()) + " cannot be trivially copied or moved. Please do " +
                       ctx->highlightError("'copy") + " or " + ctx->highlightError("'move"),
                   value.value()->fileRange);
      }
    } else {
      if (!expVal->getType()->isSame(newValue->getType())) {
        ctx->Error("Type of the provided expression is " + ctx->highlightError(expVal->getType()->toString()) +
                       " and does not match the type of the declaration which is " +
                       ctx->highlightError(declType->toString()),
                   value.value()->fileRange);
      }
      ctx->builder.CreateStore(expVal->getLLVM(), newValue->getAlloca());
    }
    return nullptr;
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