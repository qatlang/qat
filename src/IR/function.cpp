#include "./function.hpp"
#include "../ast/function.hpp"
#include "../ast/types/templated.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./qat_module.hpp"
#include "./types/function.hpp"
#include "./types/pointer.hpp"
#include "member_function.hpp"
#include "types/qat_type.hpp"
#include "types/reference.hpp"
#include "value.hpp"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include <vector>

namespace qat::IR {

LocalValue::LocalValue(const String &_name, IR::QatType *_type, bool _isVar,
                       Function *fun)
    : Value(nullptr, _type, _isVar, Nature::assignable), name(_name) {
  SHOW("Type is " << _type->toString())
  SHOW("Creating llvm::AllocaInst for " << name)
  if (_type->getLLVMType()) {
    SHOW("LLVM type is not null")
  } else {
    SHOW("LLVM type is null")
  }
  if (fun->getLLVMFunction()->getEntryBlock().getInstList().empty()) {
    ll = new llvm::AllocaInst(_type->getLLVMType(), 0U, name,
                              &fun->getLLVMFunction()->getEntryBlock());
  } else {
    llvm::Instruction *inst = nullptr;
    // NOLINTNEXTLINE(modernize-loop-convert)
    for (auto instr =
             fun->getLLVMFunction()->getEntryBlock().getInstList().begin();
         instr != fun->getLLVMFunction()->getEntryBlock().getInstList().end();
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
                                &fun->getLLVMFunction()->getEntryBlock());
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
  name = std::to_string(fn->blocks.size() - 1) + "_bb";
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
    SHOW("Local value name is: " << val->getName())
    SHOW("Local value type is: " << val->getType()->toString())
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

void Block::setActive(llvm::IRBuilder<> &builder) const {
  fn->setActiveBlock(index);
  builder.SetInsertPoint(bb);
}

Function::Function(QatModule *_mod, String _name, QatType *returnType,
                   bool _isRetTypeVariable, bool _is_async, Vec<Argument> _args,
                   bool _isVariadicArguments, utils::FileRange fileRange,
                   const utils::VisibilityInfo &_visibility_info,
                   llvm::LLVMContext &ctx, bool isMemberFn,
                   llvm::GlobalValue::LinkageTypes llvmLinkage,
                   bool                            ignoreParentName)
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
    ll =
        llvm::Function::Create((llvm::FunctionType *)(getType()->getLLVMType()),
                               llvmLinkage, 0U, name, mod->getLLVMModule());
  } else {
    ll = llvm::Function::Create(
        (llvm::FunctionType *)(getType()->getLLVMType()), llvmLinkage, 0U,
        (ignoreParentName ? name : mod->getFullNameWithChild(name)),
        mod->getLLVMModule());
  }
}

IR::Value *Function::call(IR::Context *ctx, Vec<llvm::Value *> argValues,
                          QatModule *destMod) {
  SHOW("Linking function if it is external")
  auto *llvmFunction = (llvm::Function *)ll;
  if (destMod->getID() != mod->getID()) {
    llvm::Function::Create((llvm::FunctionType *)getType()->getLLVMType(),
                           llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage,
                           llvmFunction->getAddressSpace(), getFullName(),
                           destMod->getLLVMModule());
  }
  SHOW("Getting return type")
  auto *retType = ((FunctionType *)getType())->getReturnType();
  SHOW("Getting function type")
  auto *fnTy = (llvm::FunctionType *)getType()->asFunction()->getLLVMType();
  SHOW("Got function type")
  SHOW("Creating LLVM call: " << getFullName())
  return new IR::Value(
      ctx->builder.CreateCall(fnTy, llvmFunction, argValues), retType,
      retType->isReference() && retType->asReference()->isSubtypeVariable(),
      (retType->isReference() && retType->asReference()->isSubtypeVariable())
          ? Nature::assignable
          : Nature::temporary);
}

Function *Function::Create(QatModule *mod, String name, QatType *returnTy,
                           const bool isReturnTypeVariable, const bool is_async,
                           Vec<Argument> args, const bool hasvariadicargs,
                           utils::FileRange                fileRange,
                           const utils::VisibilityInfo    &visibilityInfo,
                           llvm::LLVMContext              &ctx,
                           llvm::GlobalValue::LinkageTypes linkage,
                           bool                            ignoreParentName) {
  return new Function(mod, std::move(name), returnTy, isReturnTypeVariable,
                      is_async, std::move(args), hasvariadicargs,
                      std::move(fileRange), visibilityInfo, ctx, false, linkage,
                      ignoreParentName);
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

TemplateFunction::TemplateFunction(String                    _name,
                                   Vec<ast::TemplatedType *> _templates,
                                   ast::FunctionDefinition  *_functionDef,
                                   QatModule                *_parent,
                                   utils::VisibilityInfo     _visibInfo)
    : name(std::move(_name)), templates(std::move(_templates)),
      functionDefinition(_functionDef), parent(_parent), visibInfo(_visibInfo) {
  parent->templateFunctions.push_back(this);
}

String TemplateFunction::getName() const { return name; }

utils::VisibilityInfo TemplateFunction::getVisibility() const {
  return visibInfo;
}

usize TemplateFunction::getTypeCount() const { return templates.size(); }

usize TemplateFunction::getVariantCount() const { return variants.size(); }

QatModule *TemplateFunction::getModule() const { return parent; }

Function *TemplateFunction::fillTemplates(Vec<IR::QatType *> types,
                                          IR::Context       *ctx) {
  for (auto var : variants) {
    if (var.check(types)) {
      return var.get();
    }
  }
  for (usize i = 0; i < templates.size(); i++) {
    templates.at(i)->setType(types.at(i));
  }
  auto *fun = (IR::Function *)functionDefinition->emit(ctx);
  variants.push_back(TemplateVariant<Function>(fun, types));
  for (auto *temp : templates) {
    temp->unsetType();
  }
  return fun;
}

} // namespace qat::IR