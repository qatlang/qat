#include "./function.hpp"
#include "../ast/function.hpp"
#include "../ast/types/templated.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./logic.hpp"
#include "./qat_module.hpp"
#include "./types/function.hpp"
#include "./types/pointer.hpp"
#include "member_function.hpp"
#include "types/qat_type.hpp"
#include "types/reference.hpp"
#include "types/void.hpp"
#include "value.hpp"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include <algorithm>
#include <vector>

namespace qat::IR {

LocalValue::LocalValue(String _name, IR::QatType* _type, bool _isVar, Function* fun)
    : Value(nullptr, _type, _isVar, Nature::assignable), name(std::move(_name)) {
  SHOW("Type is " << type->toString())
  SHOW("Creating llvm::AllocaInst for " << name)
  if (type->getLLVMType()) {
    SHOW("LLVM type is not null")
  } else {
    SHOW("LLVM type is null")
  }
  ll      = IR::Logic::newAlloca(fun, name, type->getLLVMType());
  localID = utils::unique_id();
  SHOW("AllocaInst name is: " << ((llvm::AllocaInst*)ll)->getName().str());
}

String LocalValue::getName() const { return name; }

llvm::AllocaInst* LocalValue::getAlloca() const { return (llvm::AllocaInst*)ll; }

Block::Block(Function* _fn, Block* _parent) : parent(_parent), fn(_fn), index(0), active(0) {
  if (parent) {
    index = parent->children.size();
    parent->children.push_back(this);
  } else {
    index = fn->blocks.size();
    fn->blocks.push_back(this);
  }
  name = (hasParent() ? (parent->getName() + ".") : "") + std::to_string(index) + "_bb";
  SHOW("Name of the block set in function: " << fn->getFullName())
  if (fn->isAsyncFunction()) {
    bb = llvm::BasicBlock::Create(fn->getLLVMFunction()->getContext(), name, fn->getAsyncSubFunction());
  } else {
    bb = llvm::BasicBlock::Create(fn->getLLVMFunction()->getContext(), name, fn->getLLVMFunction());
  }
  SHOW("Created llvm::BasicBlock" << name)
}

String Block::getName() const { return name; }

llvm::BasicBlock* Block::getBB() const { return bb; }

bool Block::hasParent() const { return (parent != nullptr); }

bool Block::hasPrevBlock() const { return prevBlock != nullptr; }

Block* Block::getPrevBlock() const { return prevBlock; }

bool Block::hasNextBlock() const { return nextBlock != nullptr; }

Block* Block::getNextBlock() const { return nextBlock; }

void Block::linkPrevBlock(Block* block) { prevBlock = block; }

Block* Block::getParent() const { return parent; }

Function* Block::getFn() const { return fn; }

bool Block::hasValue(const String& name) const {
  SHOW("Number of local values: " << values.size())
  for (auto* val : values) {
    if (val->getName() == name) {
      SHOW("Has local with name")
      return true;
    }
    SHOW("Local value name is: " << val->getName())
    SHOW("Local value type is: " << val->getType()->toString())
  }
  if (prevBlock) {
    if (prevBlock->hasValue(name)) {
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

LocalValue* Block::getValue(const String& name) const {
  for (auto* val : values) {
    if (val->getName() == name) {
      return val;
    }
  }
  if (prevBlock && prevBlock->hasValue(name)) {
    return prevBlock->getValue(name);
  }
  if (hasParent()) {
    return parent->getValue(name);
  }
  return nullptr;
}

LocalValue* Block::newValue(const String& name, IR::QatType* type, bool isVar) {
  values.push_back(new LocalValue(name, type, isVar, fn));
  SHOW("Heap allocated LocalValue")
  return values.back();
}

bool Block::hasAlias(const String& name) const {
  for (const auto& alias : aliases) {
    if (alias.first == name) {
      return true;
    }
  }
  return false;
}

IR::Value* Block::getAlias(const String& name) const {
  for (const auto& alias : aliases) {
    if (alias.first == name) {
      return alias.second;
    }
  }
  return nullptr;
}

void Block::addAlias(String name, IR::Value* value) const {
  aliases.push_back(Pair<String, IR::Value*>(std::move(name), value));
}

void Block::setGhost(bool value) const { isGhost = value; }

void Block::setHasGive() const { hasGive = true; }

bool Block::hasGiveInAllControlPaths() const {
  if (isGhost) {
    return true;
  } else {
    if (hasGive) {
      return true;
    } else {
      if (!children.empty()) {
        bool result = true;
        for (auto* child : children) {
          if (!child->hasGiveInAllControlPaths()) {
            result = false;
            break;
          }
        }
        return result;
      } else {
        return false;
      }
    }
  }
}

void Block::setActive(llvm::IRBuilder<>& builder) {
  active = None;
  if (hasParent()) {
    parent->setActive(builder);
    parent->active = index;
  } else {
    fn->setActiveBlock(index);
  }
  builder.SetInsertPoint(bb);
}

void Block::collectAllLocalValuesSoFar(Vec<LocalValue*>& vals) const {
  if (hasParent()) {
    parent->collectAllLocalValuesSoFar(vals);
  } else {
    for (auto* val : values) {
      vals.push_back(val);
    }
  }
}

void Block::collectLocalsFrom(Vec<LocalValue*>& vals) const {
  for (auto* val : values) {
    vals.push_back(val);
  }
  for (auto* child : children) {
    child->collectLocalsFrom(vals);
  }
  if (nextBlock) {
    nextBlock->collectLocalsFrom(vals);
  }
}

Vec<LocalValue*>& Block::getLocals() { return values; }

bool Block::isMoved(const String& locID) const {
  if (hasParent()) {
    auto parentRes = parent->isMoved(locID);
    if (parentRes) {
      return parentRes;
    }
  }
  for (const auto& mov : movedValues) {
    if (mov == locID) {
      return true;
    }
  }
  if (prevBlock) {
    return prevBlock->isMoved(locID);
  }
  return false;
}

void Block::addMovedValue(String locID) const { movedValues.push_back(std::move(locID)); }

Block* Block::getActive() {
  if (active) {
    return children.at(active.value())->getActive();
  } else {
    return this;
  }
}

void Block::destroyLocals(IR::Context* ctx) {
  SHOW("Locals being destroyed for " << name)
  for (auto* loc : values) { // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
    if (loc->getType()->isCoreType()) {
      if (loc->getType()->asCore()->hasDestructor()) {
        auto* dFn = loc->getType()->asCore()->getDestructor();
        (void)dFn->call(ctx, {loc->getAlloca()}, ctx->getMod());
      }
    }
  }
  if (prevBlock) {
    prevBlock->destroyLocals(ctx);
  }
}

Function::Function(QatModule* _mod, String _name, QatType* returnType, bool _isRetTypeVariable, bool _is_async,
                   Vec<Argument> _args, bool _isVariadicArguments, utils::FileRange _fileRange,
                   const utils::VisibilityInfo& _visibility_info, llvm::LLVMContext& ctx, bool isMemberFn,
                   llvm::GlobalValue::LinkageTypes llvmLinkage, bool ignoreParentName)
    : Value(nullptr, nullptr, false, Nature::pure), name(std::move(_name)), isReturnValueVariable(_isRetTypeVariable),
      mod(_mod), arguments(std::move(_args)), visibility_info(_visibility_info), fileRange(std::move(_fileRange)),
      is_async(_is_async), hasVariadicArguments(_isVariadicArguments) //
{
  SHOW("Function name :: " << name << " ; " << this)
  Vec<ArgumentType*> argTypes;
  for (auto const& arg : arguments) {
    argTypes.push_back(
        new ArgumentType(arg.getName(), arg.getType(), arg.isMemberArg(), arg.get_variability(), arg.isRetArg()));
  }
  if (is_async) {
    SHOW("Argument type for future return is: " << returnType->toString())
    argTypes.push_back(
        new ArgumentType("qat'returnValue", IR::ReferenceType::get(true, returnType, ctx), false, false, true));
    type = new FunctionType(IR::VoidType::get(ctx), false, argTypes, ctx);
  } else {
    type = new FunctionType(returnType, _isRetTypeVariable, argTypes, ctx);
  }
  if (isMemberFn) {
    ll = llvm::Function::Create((llvm::FunctionType*)(getType()->getLLVMType()), llvmLinkage, 0U, name,
                                mod->getLLVMModule());
  } else {
    ll = llvm::Function::Create((llvm::FunctionType*)(getType()->getLLVMType()), llvmLinkage, 0U,
                                (ignoreParentName ? name : mod->getFullNameWithChild(name)), mod->getLLVMModule());
  }
  if (is_async) {
    asyncFn = llvm::Function::Create(llvm::FunctionType::get(llvm::Type::getInt8Ty(ctx)->getPointerTo(),
                                                             {llvm::Type::getInt8Ty(ctx)->getPointerTo()}, false),
                                     llvm::GlobalValue::LinkageTypes::WeakAnyLinkage, 0U,
                                     (llvm::dyn_cast<llvm::Function>(ll))->getName() + "'async", mod->getLLVMModule());
    Vec<llvm::Type*> argTys;
    for (const auto& fnArg : getType()->asFunction()->getArgumentTypes()) {
      argTys.push_back(fnArg->getType()->getLLVMType());
    }
    asyncArgTy = llvm::StructType::get(ll->getContext(), argTys);
  }
}

Function::~Function() {
  SHOW("Deleting function: " << getFullName() << " ; address " << this)
  for (auto* blk : blocks) {
    delete blk;
  }
  SHOW("Function deleted")
}

llvm::Function* Function::getAsyncSubFunction() const { return asyncFn.value(); }

llvm::StructType* Function::getAsyncArgType() const { return asyncArgTy.value(); }

IR::Value* Function::call(IR::Context* ctx, const Vec<llvm::Value*>& argValues, QatModule* destMod) {
  SHOW("Linking function if it is external")
  auto* llvmFunction = (llvm::Function*)ll;
  if (destMod->getID() != mod->getID()) {
    // FIXME - This will prevent some functions with duplicate names in the global scope to be not linked during calls
    if (!destMod->getLLVMModule()->getFunction(getFullName())) {
      llvm::Function::Create((llvm::FunctionType*)getType()->getLLVMType(),
                             llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, llvmFunction->getAddressSpace(),
                             getFullName(), destMod->getLLVMModule());
    }
  }
  bool         hasRetArg = false;
  IR::QatType* retArgTy  = nullptr;
  hasRetArg              = getType()->asFunction()->hasReturnArgument();
  if (hasRetArg) {
    retArgTy = getType()->asFunction()->getReturnArgType();
  }
  if (!hasRetArg) {
    SHOW("Getting return type")
    auto* retType = ((FunctionType*)getType())->getReturnType();
    SHOW("Getting function type")
    auto* fnTy = (llvm::FunctionType*)getType()->asFunction()->getLLVMType();
    SHOW("Got function type")
    SHOW("Creating LLVM call: " << getFullName())
    return new IR::Value(ctx->builder.CreateCall(fnTy, llvmFunction, argValues), retType,
                         retType->isReference() && retType->asReference()->isSubtypeVariable(),
                         (retType->isReference() && retType->asReference()->isSubtypeVariable()) ? Nature::assignable
                                                                                                 : Nature::temporary);
  } else {
    Vec<llvm::Value*> argVals = argValues;
    SHOW("Function: Casting return arg type to reference")
    auto* retValAlloca =
        IR::Logic::newAlloca(ctx->fn, utils::unique_id(), retArgTy->asReference()->getSubType()->getLLVMType());
    argVals.push_back(retValAlloca);
    ctx->builder.CreateCall(llvmFunction->getFunctionType(), llvmFunction, argVals);
    SHOW("Call complete for fn: " << getFullName())
    return new Value(retValAlloca, retArgTy->asReference()->getSubType(), false, IR::Nature::temporary);
  }
}

Function* Function::Create(QatModule* mod, String name, QatType* returnTy, const bool isReturnTypeVariable,
                           const bool isAsync, Vec<Argument> args, const bool hasVariadicArgs,
                           utils::FileRange fileRange, const utils::VisibilityInfo& visibilityInfo,
                           llvm::LLVMContext& ctx, llvm::GlobalValue::LinkageTypes linkage, bool ignoreParentName) {
  return new Function(mod, std::move(name), returnTy, isReturnTypeVariable, isAsync, std::move(args), hasVariadicArgs,
                      std::move(fileRange), visibilityInfo, ctx, false, linkage, ignoreParentName);
}

bool Function::hasVariadicArgs() const { return hasVariadicArguments; }

bool Function::isAsyncFunction() const { return is_async; }

String Function::argumentNameAt(u32 index) const { return arguments.at(index).getName(); }

String Function::getName() const { return name; }

String Function::getFullName() const { return mod->getFullNameWithChild(name); }

bool Function::isAccessible(const utils::RequesterInfo& req_info) const {
  return utils::Visibility::isAccessible(visibility_info, req_info);
}

utils::VisibilityInfo Function::getVisibility() const { return visibility_info; }

bool Function::isMemberFunction() const { return false; }

bool Function::hasReturnArgument() const { return getType()->asFunction()->hasReturnArgument(); }

bool Function::isReturnTypeReference() const {
  return ((FunctionType*)type)->getReturnType()->typeKind() == TypeKind::reference;
}

bool Function::isReturnTypePointer() const {
  return ((FunctionType*)type)->getReturnType()->typeKind() == TypeKind::pointer;
}

llvm::Function* Function::getLLVMFunction() { return (llvm::Function*)ll; }

void Function::setActiveBlock(usize index) const { activeBlock = index; }

Block* Function::getBlock() const { return blocks.at(activeBlock)->getActive(); }

usize& Function::getCopiedCounter() { return copiedCounter; }

usize& Function::getMovedCounter() { return movedCounter; }

TemplateFunction::TemplateFunction(String _name, Vec<ast::TemplatedType*> _templates,
                                   ast::FunctionDefinition* _functionDef, QatModule* _parent,
                                   const utils::VisibilityInfo& _visibInfo)
    : name(std::move(_name)), templates(std::move(_templates)), functionDefinition(_functionDef), parent(_parent),
      visibInfo(_visibInfo) {
  parent->templateFunctions.push_back(this);
}

String TemplateFunction::getName() const { return name; }

utils::VisibilityInfo TemplateFunction::getVisibility() const { return visibInfo; }

usize TemplateFunction::getTypeCount() const { return templates.size(); }

usize TemplateFunction::getVariantCount() const { return variants.size(); }

QatModule* TemplateFunction::getModule() const { return parent; }

Function* TemplateFunction::fillTemplates(Vec<IR::QatType*> types, IR::Context* ctx,
                                          const utils::FileRange& fileRange) {
  for (auto var : variants) {
    if (var.check(types)) {
      return var.get();
    }
  }
  for (usize i = 0; i < templates.size(); i++) {
    templates.at(i)->setType(types.at(i));
  }
  auto variantName = IR::Logic::getTemplateVariantName(name, types);
  functionDefinition->prototype->setVariantName(variantName);
  auto prevTemp       = ctx->activeTemplate;
  ctx->activeTemplate = IR::TemplateEntityMarker{variantName, IR::TemplateEntityType::function, fileRange};
  auto* fun           = (IR::Function*)functionDefinition->emit(ctx);
  variants.push_back(TemplateVariant<Function>(fun, types));
  for (auto* temp : templates) {
    temp->unsetType();
  }
  if (ctx->activeTemplate->warningCount > 0) {
    auto count          = ctx->activeTemplate->warningCount;
    ctx->activeTemplate = None;
    ctx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                     " generated while creating template function: " + ctx->highlightWarning(variantName),
                 fileRange);
  }
  ctx->activeTemplate = prevTemp;
  functionDefinition->prototype->unsetVariantName();
  return fun;
}

void functionReturnHandler(IR::Context* ctx, IR::Function* fun, const utils::FileRange& fileRange) {
  auto destructorCaller = [&]() {
    Vec<IR::LocalValue*> locals;
    fun->getBlock()->collectAllLocalValuesSoFar(locals);
    for (auto* loc : locals) {
      if (loc->getType()->isCoreType()) {
        auto* cTy        = loc->getType()->asCore();
        auto* destructor = cTy->getDestructor();
        (void)destructor->call(ctx, {loc->getAlloca()}, ctx->getMod());
      }
    }
    locals.clear();
  };
  auto* block          = fun->getBlock();
  auto* retTy          = fun->getType()->asFunction()->getReturnType();
  auto  zeroAssignSelf = [&]() {
    if (fun->isMemberFunction()) {
      if (((IR::MemberFunction*)fun)->getMemberFnType() == MemberFnType::destructor) {
        if (fun->getBlock()->hasValue("''")) {
          SHOW("Destructor self value is zero assigned")
          auto* selfVal = fun->getBlock()->getValue("''");
          ctx->builder.CreateStore(llvm::Constant::getNullValue(selfVal->getType()->getLLVMType()),
                                    selfVal->getAlloca());
        } else {
          SHOW("Destructor has no self value")
        }
      }
    }
  };
  if (block->getBB()->getInstList().empty()) {
    if (retTy->isVoid()) {
      destructorCaller();
      zeroAssignSelf();
      ctx->builder.CreateRetVoid();
    } else {
      ctx->Error("Missing given value in all control paths", fileRange);
    }
  } else {
    auto* lastInst = ((llvm::Instruction*)&block->getBB()->back());
    if (!llvm::isa<llvm::ReturnInst>(lastInst)) {
      if (retTy->isVoid()) {
        destructorCaller();
        zeroAssignSelf();
        ctx->builder.CreateRetVoid();
      } else {
        ctx->Error("Missing given value in all control paths", fileRange);
      }
    }
  }
}

void destroyLocalsFrom(IR::Context* ctx, IR::Block* block) {
  Vec<IR::LocalValue*> locals;
  block->collectLocalsFrom(locals);
  for (auto* loc : locals) {
    if (loc->getType()->isCoreType()) {
      if (loc->getType()->asCore()->hasDestructor()) {
        auto* dFn = loc->getType()->asCore()->getDestructor();
        (void)dFn->call(ctx, {loc->getAlloca()}, ctx->getMod());
      }
    }
  }
}

} // namespace qat::IR