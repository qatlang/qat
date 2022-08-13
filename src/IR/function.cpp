#include "./function.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./qat_module.hpp"
#include "./types/function.hpp"
#include "./types/pointer.hpp"
#include "member_function.hpp"
#include "types/qat_type.hpp"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include <vector>

namespace qat::IR {

LocalValue::LocalValue(String _name, IR::QatType *_type, bool _isVar,
                       Function *fn)
    : Value(nullptr, _type, _isVar, Nature::assignable), name(_name) {
  SHOW("Type is " << _type->toString())
  SHOW("Creating llvm::AllocaInst for " << _name)
  if (_type->getLLVMType()) {
    SHOW("LLVM type is not null")
  } else {
    SHOW("LLVM type is null")
  }
  if (fn->getLLVMFunction()->getEntryBlock().getInstList().empty()) {
    ll = new llvm::AllocaInst(_type->getLLVMType(), 0U, _name,
                              &fn->getLLVMFunction()->getEntryBlock());
  } else {
    llvm::Instruction *inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr =
             fn->getLLVMFunction()->getEntryBlock().getInstList().begin();
         instr != fn->getLLVMFunction()->getEntryBlock().getInstList().end();
         instr++) {
      if (llvm::isa<llvm::AllocaInst>(&*instr)) {
        continue;
      } else {
        inst = &*instr;
        break;
      }
    }
    if (inst) {
      ll = new llvm::AllocaInst(_type->getLLVMType(), 0U, _name, inst);
    } else {
      ll = new llvm::AllocaInst(_type->getLLVMType(), 0U, _name,
                                &fn->getLLVMFunction()->getEntryBlock());
    }
  }
}

String LocalValue::getName() const {
  return ((llvm::AllocaInst *)ll)->getName().str();
}

llvm::AllocaInst *LocalValue::getAlloca() const {
  return (llvm::AllocaInst *)ll;
}

Block::Block(Function *_fn, Block *_parent)
    : parent(_parent), fn(_fn), index(0) {
  SHOW("Starting block creation")
  fn->blocks.push_back(this);
  SHOW("Block pushed the block-list in function")
  index = fn->blocks.size() - 1;
  SHOW("Index of block set")
  name = std::to_string(fn->blocks.size() - 1);
  SHOW("Name of the block set")
  bb = llvm::BasicBlock::Create(fn->getLLVMFunction()->getContext(), name,
                                fn->getLLVMFunction());
  SHOW("Created llvm::BasicBlock")
}

String Block::getName() const { return name; }

llvm::BasicBlock *Block::getBB() const { return bb; }

bool Block::hasParent() const { return (parent != nullptr); }

Block *Block::getParent() const { return parent; }

Function *Block::getFn() const { return fn; }

bool Block::hasValue(const String &name) const {
  SHOW("Number of local values: " << values.size())
  for (auto *val : values) {
    if (val->getName() == name) {
      SHOW("Has local with name")
      return true;
    }
  }
  if (hasParent()) {
    SHOW("Has parent block")
    return parent->hasValue(name);
  }
  SHOW("No local with name")
  return false;
}

LocalValue *Block::getValue(const String &name) const {
  for (auto *val : values) {
    if (val->getName() == name) {
      return val;
    }
  }
  if (hasParent()) {
    return parent->getValue(name);
  }
  return nullptr;
}

LocalValue *Block::newValue(const String &name, IR::QatType *type, bool isVar) {
  values.push_back(new LocalValue(name, type, isVar, fn));
  SHOW("Heap allocated LocalValue")
  return values.back();
}

void Block::setActive(
    llvm::IRBuilder<llvm::ConstantFolder, llvm::IRBuilderDefaultInserter>
        &builder) const {
  fn->setActiveBlock(index);
  builder.SetInsertPoint(bb);
}

Function::Function(QatModule *_mod, String _name, QatType *returnType,
                   bool _isRetTypeVariable, bool _is_async, Vec<Argument> _args,
                   bool _isVariadicArguments, utils::FileRange fileRange,
                   const utils::VisibilityInfo &_visibility_info,
                   llvm::LLVMContext &ctx, bool isMemberFn)
    : Value(nullptr, nullptr, false, Nature::pure), name(std::move(_name)),
      isReturnValueVariable(_isRetTypeVariable), mod(_mod),
      arguments(std::move(_args)), visibility_info(_visibility_info),
      fileRange(std::move(fileRange)), is_async(_is_async),
      hasVariadicArguments(_isVariadicArguments) //
{
  Vec<ArgumentType *> argTypes;
  for (auto const &arg : arguments) {
    argTypes.push_back(
        new ArgumentType(arg.getName(), arg.getType(), arg.get_variability()));
  }
  type = new FunctionType(returnType, _isRetTypeVariable, argTypes, ctx);
  if (isMemberFn) {
    auto *memFn = (MemberFunction *)this;
    ll =
        llvm::Function::Create((llvm::FunctionType *)(getType()->getLLVMType()),
                               llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
                               0U, name, mod->getLLVMModule());
    switch (memFn->getMemberFnType()) {
    case MemberFnType::constructor: {
      memFn->getParentType()->constructors.push_back(memFn);
      break;
    }
    case MemberFnType::normal: {
      memFn->getParentType()->memberFunctions.push_back(memFn);
      break;
    }
    case MemberFnType::staticFn: {
      memFn->getParentType()->staticFunctions.push_back(memFn);
      break;
    }
    case MemberFnType::fromConvertor: {
      memFn->getParentType()->fromConvertors.push_back(memFn);
      break;
    }
    case MemberFnType::toConvertor: {
      memFn->getParentType()->toConvertors.push_back(memFn);
      break;
    }
    case MemberFnType::copyConstructor: {
      memFn->getParentType()->copyConstructor = memFn;
      break;
    }
    case MemberFnType::moveConstructor: {
      memFn->getParentType()->moveConstructor = memFn;
      break;
    }
    case MemberFnType::destructor: {
      memFn->getParentType()->destructor = memFn;
      break;
    }
    }
  } else {
    ll = llvm::Function::Create(
        (llvm::FunctionType *)(getType()->getLLVMType()),
        llvm::GlobalValue::LinkageTypes::WeakAnyLinkage, 0U,
        mod->getFullNameWithChild(name), mod->getLLVMModule());
  }
}

IR::Value *Function::call(IR::Context *ctx, Vec<llvm::Value *> argValues,
                          QatModule *destMod) {
  SHOW("Linking function if it is external")
  // Linking the function if it is external
  auto *llvmFunction = (llvm::Function *)ll;
  if (destMod->getLLVMModule()->getFunction(getFullName()) == nullptr) {
    llvm::Function::Create((llvm::FunctionType *)getType()->getLLVMType(),
                           llvmFunction->getLinkage(),
                           llvmFunction->getAddressSpace(), getFullName(),
                           destMod->getLLVMModule());
  }
  SHOW("Getting return type")
  auto *retType = ((FunctionType *)getType())->getReturnType();
  SHOW("Getting function type")
  auto *fnTy = (llvm::FunctionType *)getType()->asFunction()->getLLVMType();
  SHOW("Got function type")
  SHOW("Creating LLVM call")
  return new IR::Value(ctx->builder.CreateCall(fnTy, llvmFunction, argValues),
                       retType, isReturnValueVariable,
                       ((retType->isReference() || retType->isPointer()) &&
                        isReturnValueVariable)
                           ? Nature::assignable
                           : Nature::temporary);
}

Function *Function::Create(QatModule *mod, String name, QatType *returnTy,
                           const bool isReturnTypeVariable, const bool is_async,
                           Vec<Argument> args, const bool hasvariadicargs,
                           utils::FileRange             fileRange,
                           const utils::VisibilityInfo &visibilityInfo,
                           llvm::LLVMContext           &ctx) {
  return new Function(mod, std::move(name), returnTy, isReturnTypeVariable,
                      is_async, std::move(args), hasvariadicargs,
                      std::move(fileRange), visibilityInfo, ctx, false);
}

bool Function::hasVariadicArgs() const { return hasVariadicArguments; }

bool Function::isAsyncFunction() const { return is_async; }

String Function::argumentNameAt(u32 index) const {
  return arguments.at(index).getName();
}

String Function::getName() const { return name; }

String Function::getFullName() const { return mod->getFullNameWithChild(name); }

bool Function::isAccessible(const utils::RequesterInfo &req_info) const {
  return utils::Visibility::isAccessible(visibility_info, req_info);
}

utils::VisibilityInfo Function::getVisibility() const {
  return visibility_info;
}

bool Function::isMemberFunction() const { return false; }

bool Function::isReturnTypeReference() const {
  return ((FunctionType *)type)->getReturnType()->typeKind() ==
         TypeKind::reference;
}

bool Function::isReturnTypePointer() const {
  return ((FunctionType *)type)->getReturnType()->typeKind() ==
         TypeKind::pointer;
}

llvm::Function *Function::getLLVMFunction() { return (llvm::Function *)ll; }

void Function::setActiveBlock(usize index) const { activeBlock = index; }

Block *Function::getBlock() const { return blocks.at(activeBlock); }

} // namespace qat::IR