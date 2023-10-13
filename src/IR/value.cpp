#include "./value.hpp"
#include "../show.hpp"
#include "./types/qat_type.hpp"
#include "./types/reference.hpp"
#include "context.hpp"
#include "logic.hpp"
#include "types/function.hpp"
#include "types/pointer.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::IR {

Value::Value(llvm::Value* _llvmValue, IR::QatType* _type, bool _isVariable, Nature _kind)
    : type(_type), nature(_kind), variable(_isVariable), ll(_llvmValue) {
  allValues.push_back(this);
}

Vec<Value*> Value::allValues = {};

QatType* Value::getType() const { return type; }

llvm::Value* Value::getLLVM() const { return ll; }

bool Value::isLocalToFn() const { return localID.has_value(); }

String Value::getLocalID() const { return localID.value(); }

void Value::setLocalID(const String& locID) { localID = locID; }

bool Value::isImplicitPointer() const {
  return ll && (llvm::isa<llvm::AllocaInst>(ll) || llvm::isa<llvm::GlobalVariable>(ll));
}

void Value::makeImplicitPointer(IR::Context* ctx, Maybe<String> name) {
  if (!isImplicitPointer()) {
    auto* alloc = IR::Logic::newAlloca(ctx->fn, name.value_or(utils::unique_id()), ll->getType());
    ctx->builder.CreateStore(ll, alloc);
    ll = alloc;
  }
}

void Value::loadImplicitPointer(llvm::IRBuilder<>& builder) {
  if (isImplicitPointer()) {
    ll = builder.CreateLoad(getType()->getLLVMType(), ll);
  }
}

Value* Value::call(IR::Context* ctx, const Vec<llvm::Value*>& args, QatModule* mod) { // NOLINT(misc-unused-parameters)
  llvm::FunctionType* fnTy      = nullptr;
  bool                hasRetArg = false;
  IR::QatType*        retArgTy  = nullptr;
  if (type->isPointer() && type->asPointer()->getSubType()->isFunction()) {
    fnTy      = llvm::dyn_cast<llvm::FunctionType>(type->asPointer()->getSubType()->getLLVMType());
    hasRetArg = type->asPointer()->getSubType()->asFunction()->hasReturnArgument();
    if (hasRetArg) {
      retArgTy = type->asPointer()->getSubType()->asFunction()->getReturnArgType();
    }
  } else {
    fnTy      = llvm::dyn_cast<llvm::FunctionType>(getType()->getLLVMType());
    hasRetArg = type->asFunction()->hasReturnArgument();
    if (hasRetArg) {
      retArgTy = type->asFunction()->getReturnArgType();
    }
  }
  if (!hasRetArg) {
    return new Value(ctx->builder.CreateCall(fnTy, ll, args),
                     type->isPointer() ? type->asPointer()->getSubType()->asFunction()->getReturnType()
                                       : type->asFunction()->getReturnType(),
                     false, IR::Nature::temporary);
  } else {
    Vec<llvm::Value*> argVals = args;
    SHOW("Value: Casting return arg type to reference")
    auto* retValAlloca =
        IR::Logic::newAlloca(ctx->fn, utils::unique_id(), retArgTy->asReference()->getSubType()->getLLVMType());
    argVals.push_back(retValAlloca);
    ctx->builder.CreateCall(fnTy, ll, argVals);
    return new Value(retValAlloca, retArgTy, false, IR::Nature::temporary);
  }
}

bool Value::isPointer() const { return type->isPointer(); }

bool Value::isReference() const { return type->isReference(); }

bool Value::isVariable() const { return variable; }

bool Value::isLLVMConstant() const { return llvm::dyn_cast<llvm::Constant>(ll); }

llvm::Constant* Value::getLLVMConstant() const { return llvm::cast<llvm::Constant>(ll); }

bool Value::isConstVal() const { return false; }

PrerunValue* Value::asConst() const { return (PrerunValue*)this; }

Nature Value::getNature() const { return nature; }

void Value::clearAll() {
  for (auto* val : allValues) {
    delete val;
  }
  allValues.clear();
}

PrerunValue::PrerunValue(llvm::Constant* _llConst, IR::QatType* _type)
    : Value(_llConst, _type, false, IR::Nature::pure) {}

PrerunValue::PrerunValue(IR::TypedType* _typed) : Value(nullptr, _typed, false, IR::Nature::pure) {}

llvm::Constant* PrerunValue::getLLVM() const { return (llvm::Constant*)ll; }

bool PrerunValue::isConstVal() const { return true; }

bool PrerunValue::isEqualTo(PrerunValue* other) {
  if (getType()->isTyped()) {
    if (other->getType()->isTyped()) {
      return getType()->asTyped()->getSubType()->isSame(other->getType()->asTyped()->getSubType());
    } else {
      return false;
    }
  } else {
    if (asConst()->getType()->isTyped()) {
      if (other->getType()->isTyped()) {
        return asConst()->getType()->asTyped()->getSubType()->isSame(other->getType()->asTyped()->getSubType());
      } else {
        return false;
      }
    } else if (other->getType()->isTyped()) {
      return false;
    } else {
      return getType()->equalityOf(this, other).value_or(false);
    }
  }
}

} // namespace qat::IR