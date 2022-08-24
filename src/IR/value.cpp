#include "./value.hpp"
#include "./types/qat_type.hpp"
#include "llvm/IR/Instructions.h"

namespace qat::IR {

Value::Value(llvm::Value *_llvmValue, IR::QatType *_type, bool _isVariable,
             Nature _kind)
    : type(_type), nature(_kind), variable(_isVariable), ll(_llvmValue),
      isLocal(false) {
  allValues.push_back(this);
}

Vec<Value *> Value::allValues = {};

QatType *Value::getType() const { return type; }

llvm::Value *Value::getLLVM() { return ll; }

bool Value::isLocalToFn() const { return isLocal; }

void Value::setIsLocalToFn(bool isLoc) { isLocal = isLoc; }

IR::Value *Value::createAlloca(llvm::IRBuilder<> &builder) {
  auto              name  = utils::unique_id();
  auto             *cBB   = builder.GetInsertBlock();
  auto             *llFun = builder.GetInsertBlock()->getParent();
  llvm::AllocaInst *alloc = nullptr;
  if (llFun->getEntryBlock().getInstList().empty()) {
    alloc = new llvm::AllocaInst(getType()->getLLVMType(), 0U, name,
                                 &(llFun->getEntryBlock()));
  } else {
    llvm::Instruction *inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr = llFun->getEntryBlock().getInstList().begin();
         instr != llFun->getEntryBlock().getInstList().end(); instr++) {
      if (llvm::isa<llvm::AllocaInst>(&*instr)) {
        continue;
      } else {
        inst = &*instr;
        break;
      }
    }
    if (inst) {
      alloc = new llvm::AllocaInst(getType()->getLLVMType(), 0U, name, inst);
    } else {
      alloc = new llvm::AllocaInst(getType()->getLLVMType(), 0U, name,
                                   &(llFun->getEntryBlock()));
    }
  }
  builder.SetInsertPoint(cBB);
  builder.CreateStore(ll, alloc);
  return new IR::Value(alloc, getType(), variable, nature);
}

bool Value::isImplicitPointer() const {
  return ll && (llvm::isa<llvm::AllocaInst>(ll) ||
                llvm::isa<llvm::GlobalVariable>(ll));
}

void Value::loadImplicitPointer(llvm::IRBuilder<> &builder) {
  if (isImplicitPointer()) {
    ll = builder.CreateLoad(getType()->getLLVMType(), ll);
  }
}

bool Value::isPointer() const {
  return (type->typeKind() == IR::TypeKind::pointer);
}

bool Value::isReference() const {
  return (type->typeKind() == IR::TypeKind::reference);
}

bool Value::isVariable() const { return variable; }

Nature Value::getNature() const { return nature; }

void Value::clearAll() {
  for (auto *val : allValues) {
    delete val;
  }
  allValues.clear();
}

} // namespace qat::IR