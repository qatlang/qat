#include "./local_declaration.hpp"
#include "../../show.hpp"
#include "../expressions/array_literal.hpp"
#include "../expressions/constructor_call.hpp"
#include "../expressions/plain_initialiser.hpp"
#include "../expressions/union_initialiser.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::ast {

LocalDeclaration::LocalDeclaration(QatType *_type, bool _isRef, String _name,
                                   Expression *_value, bool _variability,
                                   utils::FileRange _fileRange)
    : Sentence(std::move(_fileRange)), type(_type), name(std::move(_name)),
      value(_value), variability(_variability),
      isRef(_isRef){SHOW("Name for local declaration is " << name)}

          IR::Value
          * LocalDeclaration::emit(IR::Context * ctx) {
  auto *block = ctx->fn->getBlock();
  if (block->hasValue(name)) {
    ctx->Error("A local value named " + ctx->highlightError(name) +
                   " already exists in this scope. Please change name of this "
                   "declaration or check the logic",
               fileRange);
  }
  IR::QatType *declType = nullptr;

  // EDGE CASE -> The following code avoids multiple allocations for arrays and
  // core types
  if (value && (value->nodeType() == NodeType::arrayLiteral)) {
    auto *arr = (ast::ArrayLiteral *)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isArray()) {
        ctx->Error("The type for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       ", but the provided value is not compatible",
                   value->fileRange);
      }
      auto *loc  = block->newValue(name, declType, variability);
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
    auto *plain = (ast::PlainInitialiser *)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isCoreType()) {
        ctx->Error("The type provided for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       " and is not a core type. So the value provided cannot "
                       "be a plain initialiser",
                   value->fileRange);
      }
      if (!(plain->type)) {
        if (type->typeKind() == TypeKind::named) {
          plain->type = (NamedType *)type;
        } else {
          ctx->Error("Invalid type provided for plain initialisation",
                     type->fileRange);
        }
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto *plainTy = plain->type->emit(ctx);
      if (declType->isSame(plainTy)) {
        auto *loc    = block->newValue(name, declType, variability);
        plain->local = loc;
        (void)plain->emit(ctx);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " +
                       ctx->highlightError(plainTy->toString()),
                   value->fileRange);
      }
    } else {
      plain->irName = name;
      plain->isVar  = variability;
      (void)plain->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::constructorCall) &&
             !((((ast::ConstructorCall *)value))->isHeaped)) {
    auto *cons = (ast::ConstructorCall *)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isCoreType()) {
        ctx->Error("The type provided for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       " and is not a core type",
                   fileRange);
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto *constrTy = cons->type->emit(ctx);
      if (declType->isSame(constrTy)) {
        auto *loc   = block->newValue(name, declType, variability);
        cons->local = loc;
        (void)cons->emit(ctx);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " +
                       ctx->highlightError(constrTy->toString()),
                   fileRange);
      }
    } else {
      cons->irName = name;
      cons->isVar  = variability;
      (void)cons->emit(ctx);
      return nullptr;
    }
  } else if (value && (value->nodeType() == NodeType::unionInitialiser)) {
    auto *unionIn = (ast::UnionInitialiser *)value;
    if (type) {
      declType = type->emit(ctx);
      if (!declType->isCoreType()) {
        ctx->Error("The type provided for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       " and is not a core type",
                   fileRange);
      }
      // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
      auto *constrTy = unionIn->type->emit(ctx);
      if (declType->isSame(constrTy)) {
        auto *loc      = block->newValue(name, declType, variability);
        unionIn->local = loc;
        (void)unionIn->emit(ctx);
        return nullptr;
      } else {
        ctx->Error("The type provided for this declaration is " +
                       ctx->highlightError(declType->toString()) +
                       ", but the value provided is of type " +
                       ctx->highlightError(constrTy->toString()),
                   fileRange);
      }
    } else {
      unionIn->irName = name;
      unionIn->isVar  = variability;
      (void)unionIn->emit(ctx);
      return nullptr;
    }
  }

  // EDGE CASE ends here

  IR::Value *expVal = nullptr;
  if (value) {
    expVal = value->emit(ctx);
    SHOW("Type of value to be assigned to local value "
         << name << " is " << expVal->getType()->toString())
  }
  if (type) {
    if (!declType) {
      declType = type->emit(ctx);
    }
    if (value &&
        ((declType->isReference() && !expVal->isReference())
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
      }
    } else {
      ctx->Error("Type inference for declarations require a value", fileRange);
    }
  }
  if (declType->isReference() &&
      ((!expVal->getType()->isReference()) && expVal->isImplicitPointer())) {
    if (declType->asReference()->isSubtypeVariable() &&
        (!expVal->isVariable())) {
      ctx->Error(
          "The referred type of the left hand side has variability, but the "
          "value provided for initialisation do not have variability",
          value->fileRange);
    }
  } else if (declType->isReference() && expVal->getType()->isReference()) {
    if (declType->asReference()->isSubtypeVariable() &&
        (!expVal->getType()->asReference()->isSubtypeVariable())) {
      ctx->Error("The reference on the left hand side refers to a value with "
                 "variability, but the value provided for initialisation is a "
                 "reference that refers to a value without variability",
                 value->fileRange);
    }
  }
  auto *newValue =
      block->newValue(name, declType, type ? type->isVariable() : variability);
  if (expVal) {
    SHOW("Creating store")
    if ((expVal->isImplicitPointer() && !declType->isReference()) ||
        (expVal->isImplicitPointer() && declType->isReference() &&
         expVal->getType()->isReference())) {
      expVal->loadImplicitPointer(ctx->builder);
    } else if (!declType->isReference() && expVal->getType()->isReference()) {
      expVal = new IR::Value(
          ctx->builder.CreateLoad(declType->getLLVMType(), expVal->getLLVM()),
          declType, false, IR::Nature::temporary);
    }
    ctx->builder.CreateStore(expVal->getLLVM(), newValue->getAlloca());
    SHOW("llvm::StoreInst created")
  }
  return nullptr;
}

nuo::Json LocalDeclaration::toJson() const {
  return nuo::Json()
      ._("nodeType", "localDeclaration")
      ._("name", name)
      ._("isVariable", variability)
      ._("hasType", (type != nullptr))
      ._("type", (type != nullptr) ? type->toJson() : nuo::Json())
      ._("value", value->toJson())
      ._("fileRange", fileRange);
}

} // namespace qat::ast