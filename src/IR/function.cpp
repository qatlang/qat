#include "./function.hpp"
#include "../ast/function.hpp"
#include "../ast/types/generic_abstract.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./generics.hpp"
#include "./logic.hpp"
#include "./qat_module.hpp"
#include "./types/function.hpp"
#include "./types/pointer.hpp"
#include "./types/region.hpp"
#include "control_flow.hpp"
#include "link_names.hpp"
#include "member_function.hpp"
#include "meta_info.hpp"
#include "types/qat_type.hpp"
#include "types/reference.hpp"
#include "types/unsigned.hpp"
#include "value.hpp"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include <vector>

namespace qat::IR {

LocalValue::LocalValue(String _name, IR::QatType* _type, bool _isVar, Function* fun, FileRange _fileRange)
    : Value(nullptr, _type, _isVar, Nature::assignable), EntityOverview("localValue",
                                                                        Json()
                                                                            ._("name", _name)
                                                                            ._("typeID", _type->getID())
                                                                            ._("type", _type->toString())
                                                                            ._("isVariable", _isVar)
                                                                            ._("functionID", fun->getID()),
                                                                        _fileRange),
      name(std::move(_name)), fileRange(std::move(_fileRange)) {
  SHOW("Type is " << type->toString())
  SHOW("Creating llvm::AllocaInst for " << name)
  ll      = IR::Logic::newAlloca(fun, name, type->getLLVMType());
  localID = utils::unique_id();
  SHOW("AllocaInst name is: " << ((llvm::AllocaInst*)ll)->getName().str());
}

String LocalValue::getName() const { return name; }

llvm::AllocaInst* LocalValue::getAlloca() const { return (llvm::AllocaInst*)ll; }

FileRange LocalValue::getFileRange() const { return fileRange; }

IR::Value* LocalValue::toNewIRValue() const {
  auto* result = new IR::Value(getLLVM(), getType(), isVariable(), getNature());
  result->setLocalID(getLocalID().value());
  return result;
}

Block::Block(Function* _fn, Block* _parent) : parent(_parent), fn(_fn), index(0) {
  if (parent) {
    index = parent->children.size();
    parent->children.push_back(this);
  } else {
    index = fn->blocks.size();
    fn->blocks.push_back(this);
  }
  name = (hasParent() ? (parent->getName() + ".") : "") + std::to_string(index) + "_bb";
  bb   = llvm::BasicBlock::Create(fn->getLLVMFunction()->getContext(), name, fn->getLLVMFunction());
  SHOW("Created llvm::BasicBlock " << name)
}

String Block::getName() const { return name; }

llvm::BasicBlock* Block::getBB() const { return bb; }

bool Block::hasParent() const { return (parent != nullptr); }

bool Block::hasPrevBlock() const { return prevBlock != nullptr; }

Block* Block::getPrevBlock() const { return prevBlock; }

bool Block::hasNextBlock() const { return nextBlock != nullptr; }

Block* Block::getNextBlock() const { return nextBlock; }

void Block::linkPrevBlock(Block* block) {
  prevBlock        = block;
  block->nextBlock = this;
}

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

LocalValue* Block::newValue(const String& name, IR::QatType* type, bool isVar, FileRange _fileRange) {
  values.push_back(new LocalValue(name, type, isVar, fn, std::move(_fileRange)));
  return values.back();
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
  SHOW("Setting active block: " << getName())
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
  }
  if (hasPrevBlock()) {
    prevBlock->collectAllLocalValuesSoFar(vals);
  }
  for (auto* val : values) {
    bool exists = false;
    for (auto* collected : vals) {
      if (collected->getLocalID() == val->getLocalID()) {
        exists = true;
        break;
      }
    }
    if (!exists) {
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
  if (prevBlock) {
    return prevBlock->isMoved(locID);
  }
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
  return false;
}

Maybe<FileRange> Block::getFileRange() const {
  if (fileRange.has_value()) {
    return fileRange;
  }
  if (parent && parent->fileRange.has_value()) {
    return parent->fileRange;
  }
  return None;
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
  for (auto* loc : values) {
    if (loc->getType()->isDestructible()) {
      if (loc->getType()->isPointer() ? (loc->getType()->asPointer()->getOwner().isParentFunction() &&
                                         (loc->getType()->asPointer()->getOwner().ownerAsParentFunction()->getID() ==
                                          ctx->getActiveFunction()->getID()))
                                      : true) {
        loc->getType()->destroyValue(ctx, loc, ctx->getActiveFunction());
      }
    }
  }
  if (nextBlock) {
    nextBlock->destroyLocals(ctx);
  }
}

void Block::outputLocalOverview(Vec<JsonValue>& jsonVals) {
  for (auto* loc : values) {
    jsonVals.push_back(loc->overviewToJson());
  }
  for (auto* child : children) {
    child->outputLocalOverview(jsonVals);
  }
}

void Block::setFileRange(FileRange _fileRange) { fileRange = _fileRange; }

Function::Function(QatModule* _mod, Identifier _name, Maybe<LinkNames> _namingInfo, Vec<GenericParameter*> _generics,
                   ReturnType* returnType, Vec<Argument> _args, bool _isVariadicArguments, Maybe<FileRange> _fileRange,
                   const VisibilityInfo& _visibility_info, IR::Context* _ctx, bool isMemberFn,
                   Maybe<llvm::GlobalValue::LinkageTypes> llvmLinkage, Maybe<MetaInfo> _metaInfo)
    : Value(nullptr, nullptr, false, Nature::pure), EntityOverview("function", Json(), _name.range),
      name(std::move(_name)), namingInfo(_namingInfo.value_or(LinkNames({}, None, _mod))),
      generics(std::move(_generics)), mod(_mod), arguments(std::move(_args)), visibility_info(_visibility_info),
      fileRange(std::move(_fileRange)), hasVariadicArguments(_isVariadicArguments), metaInfo(_metaInfo), ctx(_ctx) //
{
  Maybe<String> foreignID;
  Maybe<String> linkAlias;
  if (metaInfo) {
    foreignID = metaInfo->getForeignID();
    linkAlias = metaInfo->getValueAsStringFor(IR::MetaInfo::linkAsKey);
  }
  if (!foreignID.has_value()) {
    foreignID = getParentModule()->getRelevantForeignID();
  }
  if (!_namingInfo.has_value()) {
    namingInfo = mod->getLinkNames().newWith(LinkNameUnit(name.value, LinkUnitType::function), foreignID);
    if (isGeneric()) {
      Vec<LinkNames> genericLinkNames;
      for (auto* param : generics) {
        if (param->isTyped()) {
          genericLinkNames.push_back(LinkNames(
              {LinkNameUnit(param->asTyped()->getType()->getNameForLinking(), LinkUnitType::genericTypeValue)}, None,
              nullptr));
        } else if (param->isPrerun()) {
          auto preRes = param->asPrerun()->getExpression();
          genericLinkNames.push_back(LinkNames({LinkNameUnit(preRes->getType()->toPrerunGenericString(preRes).value(),
                                                             LinkUnitType::genericPrerunValue)},
                                               None, nullptr));
        }
      }
      namingInfo.addUnit(LinkNameUnit("", LinkUnitType::genericList, genericLinkNames), None);
    }
  }
  namingInfo.setLinkAlias(linkAlias);
  linkingName = namingInfo.toName();
  Vec<ArgumentType*> argTypes;
  for (auto const& arg : arguments) {
    argTypes.push_back(new ArgumentType(arg.getName().value, arg.getType(), arg.isMemberArg(), arg.get_variability()));
  }
  type = new FunctionType(returnType, argTypes, ctx->llctx);
  if (isMemberFn) {
    ll = llvm::Function::Create(llvm::cast<llvm::FunctionType>(getType()->getLLVMType()),
                                llvmLinkage.value_or(DEFAULT_FUNCTION_LINKAGE), 0U, linkingName, mod->getLLVMModule());
  } else {
    ll = llvm::Function::Create(llvm::cast<llvm::FunctionType>(getType()->getLLVMType()),
                                llvmLinkage.value_or(DEFAULT_FUNCTION_LINKAGE), 0U, linkingName, mod->getLLVMModule());
  }
  if (!isMemberFunction() && !isGeneric()) {
    mod->functions.push_back(this);
  }
}

Function::~Function() {
  for (auto* blk : blocks) {
    delete blk;
  }
  for (auto* gen : generics) {
    delete gen;
  }
}

bool Function::isGeneric() const { return !generics.empty(); }

bool Function::hasDefinitionRange() const { return fileRange.has_value(); }

FileRange Function::getDefinitionRange() const { return fileRange.value(); }

IR::Value* Function::call(IR::Context* ctx, const Vec<llvm::Value*>& argValues, Maybe<String> localID,
                          QatModule* destMod) {
  SHOW("Linking function if it is external")
  auto* llvmFunction = llvm::cast<llvm::Function>(ll);
  if (destMod->getID() != mod->getID()) {
    // FIXME - This will prevent some functions with duplicate names in the global scope to be not linked during calls
    if (!destMod->getLLVMModule()->getFunction(llvmFunction->getName())) {
      llvm::Function::Create((llvm::FunctionType*)getType()->getLLVMType(),
                             llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, llvmFunction->getAddressSpace(),
                             llvmFunction->getName(), destMod->getLLVMModule());
    }
  }
  SHOW("Getting return type")
  auto* retType = ((FunctionType*)getType())->getReturnType();
  SHOW("Getting function type")
  auto* fnTy = (llvm::FunctionType*)getType()->asFunction()->getLLVMType();
  SHOW("Got function type")
  SHOW("Creating LLVM call: " << linkingName << " with ID " << getID())
  SHOW("Number of args: " << argValues.size())
  SHOW("Return type is " << retType->getType()->toString())
  auto result = new IR::Value(ctx->builder.CreateCall(fnTy, llvmFunction, argValues), retType->getType(), false,
                              Nature::temporary);
  if (getParentModule()->getID() != destMod->getID() && !getParentModule()->isParentModuleOf(destMod)) {
    destMod->addDependency(getParentModule());
  }
  if (localID && retType->isReturnSelf()) {
    result->setLocalID(localID.value());
  }
  return result;
}

Function* Function::Create(QatModule* mod, Identifier name, Maybe<LinkNames> namingInfo,
                           Vec<GenericParameter*> _generics, ReturnType* returnTy, Vec<Argument> args,
                           const bool hasVariadicArgs, Maybe<FileRange> fileRange, const VisibilityInfo& visibilityInfo,
                           IR::Context* ctx, Maybe<llvm::GlobalValue::LinkageTypes> linkage, Maybe<MetaInfo> metaInfo) {
  return new Function(mod, std::move(name), namingInfo, std::move(_generics), returnTy, std::move(args),
                      hasVariadicArgs, std::move(fileRange), visibilityInfo, ctx, false, linkage, metaInfo);
}

void Function::updateOverview() {
  Vec<JsonValue> localsJson;
  for (auto* block : blocks) {
    block->outputLocalOverview(localsJson);
  }
  ovInfo._("fullName", getFullName())
      ._("functionID", getID())
      ._("moduleID", mod->getID())
      ._("visibility", visibility_info)
      ._("isVariadic", hasVariadicArguments)
      ._("locals", localsJson);
}

bool Function::hasVariadicArgs() const { return hasVariadicArguments; }

Identifier Function::argumentNameAt(u32 index) const { return arguments.at(index).getName(); }

Identifier Function::getName() const { return name; }

IR::QatModule* Function::getParentModule() const { return mod; }

bool Function::hasGenericParameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->getName().value == name) {
      return true;
    }
  }
  return false;
}

String Function::getRandomAllocaName() const {
  localNameCounter++;
  return std::to_string(localNameCounter) + "_new";
}

GenericParameter* Function::getGenericParameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->getName().value == name) {
      return gen;
    }
  }
  return nullptr;
}

String Function::getFullName() const { return mod->getFullNameWithChild(name.value); }

bool Function::isAccessible(const AccessInfo& req_info) const {
  return Visibility::isAccessible(visibility_info, req_info);
}

IR::LocalValue* Function::getFunctionCommonIndex() {
  if (!strComparisonIndex) {
    strComparisonIndex = getFirstBlock()->newValue("qat'function'commonIndex",
                                                   // NOLINTNEXTLINE(readability-magic-numbers)
                                                   IR::UnsignedType::get(64u, ctx), true, name.range);
  }
  return strComparisonIndex;
}

VisibilityInfo Function::getVisibility() const { return visibility_info; }

bool Function::isMemberFunction() const { return false; }

llvm::Function* Function::getLLVMFunction() { return (llvm::Function*)ll; }

void Function::setActiveBlock(usize index) const { activeBlock = index; }

Block* Function::getBlock() const { return blocks.at(activeBlock)->getActive(); }

Block* Function::getFirstBlock() const { return blocks.at(0); };

usize Function::getBlockCount() const { return blocks.size(); }

usize& Function::getCopiedCounter() { return copiedCounter; }

usize& Function::getMovedCounter() { return movedCounter; }

GenericFunction::GenericFunction(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                 Maybe<ast::PrerunExpression*> _constraint, ast::FunctionDefinition* _functionDef,
                                 QatModule* _parent, const VisibilityInfo& _visibInfo)
    : EntityOverview("genericFunction",
                     Json()
                         ._("fullName", _parent->getFullNameWithChild(_name.value))
                         ._("moduleID", _parent->getID())
                         ._("visibility", _visibInfo),
                     _name.range),
      name(std::move(_name)), generics(std::move(_generics)), functionDefinition(_functionDef), constraint(_constraint),
      parent(_parent), visibInfo(_visibInfo) {
  parent->genericFunctions.push_back(this);
}

Identifier GenericFunction::getName() const { return name; }

VisibilityInfo GenericFunction::getVisibility() const { return visibInfo; }

usize GenericFunction::getTypeCount() const { return generics.size(); }

usize GenericFunction::getVariantCount() const { return variants.size(); }

QatModule* GenericFunction::getModule() const { return parent; }

ast::GenericAbstractType* GenericFunction::getGenericAt(usize index) const { return generics.at(index); }

useit bool GenericFunction::allTypesHaveDefaults() const {
  for (auto* gen : generics) {
    if (!gen->hasDefault()) {
      return false;
    }
  }
  return true;
}

Function* GenericFunction::fillGenerics(Vec<IR::GenericToFill*> types, IR::Context* ctx, const FileRange& fileRange) {
  auto oldFn = ctx->setActiveFunction(nullptr);
  for (auto var : variants) {
    if (var.check(
            ctx, [&](const String& msg, const FileRange& rng) { ctx->Error(msg, rng); }, types)) {
      return var.get();
    }
  }
  IR::fillGenerics(ctx, generics, types, fileRange);
  if (constraint.has_value()) {
    auto checkVal = constraint.value()->emit(ctx);
    if (checkVal->getType()->isBool()) {
      if (!llvm::cast<llvm::ConstantInt>(checkVal->getLLVMConstant())->getValue().getBoolValue()) {
        ctx->Error("The provided generic parameters for the generic function do not satisfy the constraints", fileRange,
                   Pair<String, FileRange>{"The constraint can be found here", constraint.value()->fileRange});
      }
    } else {
      ctx->Error("The constraints for generic parameters should be of " + ctx->highlightError("bool") +
                     " type. Got an expression of " + ctx->highlightError(checkVal->getType()->toString()),
                 constraint.value()->fileRange);
    }
  }
  auto variantName = IR::Logic::getGenericVariantName(name.value, types);
  functionDefinition->prototype->setVariantName(variantName);
  auto prevTemp = ctx->allActiveGenerics;
  ctx->addActiveGeneric(IR::GenericEntityMarker{variantName, IR::GenericEntityType::function, fileRange}, true);
  auto* fun = (IR::Function*)functionDefinition->emit(ctx);
  variants.push_back(GenericVariant<Function>(fun, types));
  for (auto* temp : generics) {
    temp->unset();
  }
  functionDefinition->prototype->unsetVariantName();
  if (ctx->getActiveGeneric().warningCount > 0) {
    auto count = ctx->getActiveGeneric().warningCount;
    ctx->removeActiveGeneric();
    ctx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                     " generated while creating generic function: " + ctx->highlightWarning(variantName),
                 fileRange);
  } else {
    ctx->removeActiveGeneric();
  }
  (void)ctx->setActiveFunction(oldFn);
  return fun;
}

void destructorCaller(IR::Context* ctx, IR::Function* fun) {
  SHOW("DestructorCaller")
  Vec<IR::LocalValue*> locals;
  fun->getBlock()->collectAllLocalValuesSoFar(locals);
  SHOW(locals.size() << " locals collected so far")
  for (auto* loc : locals) {
    SHOW("Local name is: " << loc->getName() << " and type is " << loc->getType()->toString())
    if (loc->isReference()) {
      continue;
    }
    if (loc->getType()->isExpanded() && loc->getType()->asExpanded()->hasDestructor()) {
      auto* eTy        = loc->getType()->asExpanded();
      auto* destructor = eTy->getDestructor();
      (void)destructor->call(ctx, {loc->getAlloca()}, None, ctx->getMod());
    } else if (loc->getType()->isDestructible()) {
      loc->getType()->destroyValue(ctx, loc->toNewIRValue(), fun);
      SHOW("Destroyed value using type level feature")
    } else if (loc->getType()->hasDefaultSkill()) {
      // FIXME - Implement
      // loc->getType()->getDefaultSkill();
    } else if (loc->getType()->isPointer() && loc->getType()->asPointer()->getOwner().isParentFunction() &&
               loc->getType()->asPointer()->getOwner().ownerAsParentFunction()->getID() == fun->getID()) {
      auto* ptrTy = loc->getType()->asPointer();
      if (ptrTy->getSubType()->isCoreType() && ptrTy->getSubType()->asCore()->hasDestructor()) {
        auto* dstrFn = ptrTy->getSubType()->asCore()->getDestructor();
        if (ptrTy->isMulti()) {
          auto* currBlock = ctx->getActiveFunction()->getBlock();
          auto* condBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
          auto* trueBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
          auto* restBlock = new IR::Block(ctx->getActiveFunction(), nullptr);
          restBlock->linkPrevBlock(currBlock);
          // NOLINTNEXTLINE(readability-magic-numbers)
          auto* count = currBlock->newValue(utils::unique_id(), IR::UnsignedType::get(64u, ctx), true, {""});
          ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u, false),
                                   count->getLLVM());
          ctx->builder.CreateCondBr(
              ctx->builder.CreateICmpNE(
                  ctx->builder.CreatePtrDiff(
                      ptrTy->getSubType()->getLLVMType(),
                      ctx->builder.CreateLoad(ptrTy->getSubType()->getLLVMType()->getPointerTo(),
                                              ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), loc->getLLVM(), 0u)),
                      llvm::ConstantPointerNull::get(ptrTy->getSubType()->getLLVMType()->getPointerTo())),
                  llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
              condBlock->getBB(), restBlock->getBB());
          condBlock->setActive(ctx->builder);
          SHOW("Set condition block active")
          ctx->builder.CreateCondBr(
              ctx->builder.CreateICmpULT(
                  ctx->builder.CreateLoad(count->getType()->getLLVMType(), count->getLLVM()),
                  ctx->builder.CreateLoad(count->getType()->getLLVMType(),
                                          ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), loc->getLLVM(), 1u))),
              trueBlock->getBB(), restBlock->getBB());
          trueBlock->setActive(ctx->builder);
          SHOW("Set trueblock active")
          (void)dstrFn->call(
              ctx,
              {ctx->builder.CreateLoad(ptrTy->getSubType()->getLLVMType()->getPointerTo(),
                                       ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), loc->getLLVM(), 0u))},
              None, ctx->getMod());
          ctx->builder.CreateStore(
              ctx->builder.CreateAdd(llvm::ConstantInt::get(count->getType()->getLLVMType(), 1u, false),
                                     ctx->builder.CreateLoad(count->getType()->getLLVMType(), count->getLLVM())),
              count->getLLVM());
          (void)IR::addBranch(ctx->builder, condBlock->getBB());
          restBlock->setActive(ctx->builder);
        } else {
          auto* currBlock = ctx->getActiveFunction()->getBlock();
          auto* trueBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
          auto* restBlock = new IR::Block(ctx->getActiveFunction(), nullptr);
          restBlock->linkPrevBlock(currBlock);
          ctx->builder.CreateCondBr(
              ctx->builder.CreateICmpNE(
                  ctx->builder.CreatePtrDiff(
                      ptrTy->getLLVMType(), ctx->builder.CreateLoad(ptrTy->getLLVMType(), loc->getLLVM()),
                      llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(ptrTy->getLLVMType()))),
                  llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
              trueBlock->getBB(), restBlock->getBB());
          trueBlock->setActive(ctx->builder);
          ctx->builder.CreateCall(dstrFn->getLLVMFunction()->getFunctionType(), dstrFn->getLLVMFunction(),
                                  {ctx->builder.CreateLoad(ptrTy->getLLVMType(), loc->getLLVM())});
          (void)IR::addBranch(ctx->builder, restBlock->getBB());
          restBlock->setActive(ctx->builder);
        }
      }
      ctx->getMod()->linkNative(NativeUnit::free);
      auto* freeFn = ctx->getMod()->getLLVMModule()->getFunction("free");
      ctx->builder.CreateCall(
          freeFn->getFunctionType(), freeFn,
          {ptrTy->isMulti()
               ? ctx->builder.CreatePointerCast(
                     ctx->builder.CreateLoad(ptrTy->getSubType()->getLLVMType()->getPointerTo(),
                                             ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), loc->getLLVM(), 0u)),
                     llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())
               : ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(ptrTy->getLLVMType(), loc->getLLVM()),
                                                llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
    }
  }
  locals.clear();
}

void memberFunctionHandler(IR::Context* ctx, IR::Function* fun) {
  if (fun->isMemberFunction()) {
    auto* mFn = (IR::MemberFunction*)fun;
    // FIXME - Change this
    if (!mFn->getParentType()->isCoreType()) {
      return;
    }
    auto* cTy = mFn->getParentType()->asCore();
    if (mFn->getMemberFnType() == MemberFnType::destructor) {
      for (usize i = 0; i < cTy->getMembers().size(); i++) {
        auto& mem = cTy->getMembers().at(i);
        if (mem->type->isPointer() && mem->type->asPointer()->getOwner().isType() &&
            (mem->type->asPointer()->getOwner().ownerAsType()->getID() == mem->type->getID())) {
          auto* ptrTy  = mem->type->asPointer();
          auto* memPtr = ctx->builder.CreateStructGEP(
              ptrTy->getLLVMType(),
              ctx->builder.CreateStructGEP(cTy->getLLVMType(), mFn->getFirstBlock()->getValue("''")->getLLVM(), i), 0u);
          if (ptrTy->getSubType()->isCoreType() && ptrTy->getSubType()->asCore()->hasDestructor()) {
            auto* dstrFn = ptrTy->getSubType()->asCore()->getDestructor();
            if (ptrTy->isMulti()) {
              auto* currBlock = ctx->getActiveFunction()->getBlock();
              auto* condBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
              auto* trueBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
              auto* restBlock = new IR::Block(ctx->getActiveFunction(), nullptr);
              restBlock->linkPrevBlock(currBlock);
              // NOLINTNEXTLINE(readability-magic-numbers)
              auto* count = currBlock->newValue(utils::unique_id(), IR::UnsignedType::get(64u, ctx), true, {""});
              ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u, false),
                                       count->getLLVM());
              ctx->builder.CreateCondBr(
                  ctx->builder.CreateICmpNE(
                      ctx->builder.CreatePtrDiff(
                          ptrTy->getSubType()->getLLVMType(),
                          ctx->builder.CreateLoad(ptrTy->getSubType()->getLLVMType()->getPointerTo(),
                                                  ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), memPtr, 0u)),
                          llvm::ConstantPointerNull::get(ptrTy->getSubType()->getLLVMType()->getPointerTo())),
                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
                  condBlock->getBB(), restBlock->getBB());
              condBlock->setActive(ctx->builder);
              SHOW("Set condition block active")
              ctx->builder.CreateCondBr(
                  ctx->builder.CreateICmpULT(
                      ctx->builder.CreateLoad(count->getType()->getLLVMType(), count->getLLVM()),
                      ctx->builder.CreateLoad(count->getType()->getLLVMType(),
                                              ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), memPtr, 1u))),
                  trueBlock->getBB(), restBlock->getBB());
              trueBlock->setActive(ctx->builder);
              SHOW("Set trueblock active")
              (void)dstrFn->call(
                  ctx,
                  {ctx->builder.CreateLoad(ptrTy->getSubType()->getLLVMType()->getPointerTo(),
                                           ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), memPtr, 0u))},
                  None, ctx->getMod());
              ctx->builder.CreateStore(
                  ctx->builder.CreateAdd(llvm::ConstantInt::get(count->getType()->getLLVMType(), 1u, false),
                                         ctx->builder.CreateLoad(count->getType()->getLLVMType(), count->getLLVM())),
                  count->getLLVM());
              (void)IR::addBranch(ctx->builder, condBlock->getBB());
              restBlock->setActive(ctx->builder);
            } else {
              auto* currBlock = ctx->getActiveFunction()->getBlock();
              auto* trueBlock = new IR::Block(ctx->getActiveFunction(), currBlock);
              auto* restBlock = new IR::Block(ctx->getActiveFunction(), nullptr);
              restBlock->linkPrevBlock(currBlock);
              ctx->builder.CreateCondBr(
                  ctx->builder.CreateICmpNE(
                      ctx->builder.CreatePtrDiff(
                          ptrTy->getLLVMType(), ctx->builder.CreateLoad(ptrTy->getLLVMType(), memPtr),
                          llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(ptrTy->getLLVMType()))),
                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(ctx->llctx), 0u)),
                  trueBlock->getBB(), restBlock->getBB());
              trueBlock->setActive(ctx->builder);
              ctx->builder.CreateCall(dstrFn->getLLVMFunction()->getFunctionType(), dstrFn->getLLVMFunction(),
                                      {ctx->builder.CreateLoad(ptrTy->getLLVMType(), memPtr)});
              (void)IR::addBranch(ctx->builder, restBlock->getBB());
              restBlock->setActive(ctx->builder);
            }
          }
          ctx->getMod()->linkNative(NativeUnit::free);
          auto* freeFn = ctx->getMod()->getLLVMModule()->getFunction("free");
          ctx->builder.CreateCall(
              freeFn->getFunctionType(), freeFn,
              {ptrTy->isMulti()
                   ? ctx->builder.CreatePointerCast(
                         ctx->builder.CreateLoad(ptrTy->getSubType()->getLLVMType()->getPointerTo(),
                                                 ctx->builder.CreateStructGEP(ptrTy->getLLVMType(), memPtr, 0u)),
                         llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())
                   : ctx->builder.CreatePointerCast(ctx->builder.CreateLoad(ptrTy->getLLVMType(), memPtr),
                                                    llvm::Type::getInt8Ty(ctx->llctx)->getPointerTo())});
        }
      }
      if (fun->getBlockCount() >= 1 && fun->getFirstBlock()->hasValue("''")) {
        SHOW("Destructor self value is zero assigned")
        auto* selfVal = fun->getFirstBlock()->getValue("''");
        ctx->builder.CreateStore(llvm::Constant::getNullValue(cTy->getLLVMType()), selfVal->getAlloca());
      } else {
        SHOW("Destructor has no self value")
      }
    }
  }
}

void functionReturnHandler(IR::Context* ctx, IR::Function* fun, const FileRange& fileRange) {
  SHOW("Starting function return handle for: " << fun->getFullName())
  // FIXME - Support destructors for types besides core types
  auto* block = fun->getBlock();
  auto* retTy = fun->getType()->asFunction()->getReturnType();
  if (block->getBB()->empty()) {
    SHOW("Empty instruction list in block")
    if (retTy->getType()->isVoid()) {
      SHOW("Calling destructor caller")
      destructorCaller(ctx, fun);
      SHOW("Calling member function caller")
      memberFunctionHandler(ctx, fun);
      if (fun->getFullName() == "main") {
        for (auto* reg : QatType::allRegions()) {
          reg->destroyObjects(ctx);
        }
      }
      ctx->builder.CreateRetVoid();
    } else {
      ctx->Error("Missing given value in all control paths", fileRange);
    }
  } else {
    auto* lastInst = ((llvm::Instruction*)&block->getBB()->back());
    if (!llvm::isa<llvm::ReturnInst>(lastInst)) {
      SHOW("Last instruction is not a return")
      if (retTy->getType()->isVoid()) {
        SHOW("Calling destructor caller")
        destructorCaller(ctx, fun);
        SHOW("Calling member function handler")
        memberFunctionHandler(ctx, fun);
        if (fun->getFullName() == "main") {
          for (auto* reg : QatType::allRegions()) {
            reg->destroyObjects(ctx);
          }
        }
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
    if (loc->getType()->isDestructible()) {
      if (loc->getType()->isPointer() ? (loc->getType()->asPointer()->getOwner().isParentFunction() &&
                                         (loc->getType()->asPointer()->getOwner().ownerAsParentFunction()->getID() ==
                                          ctx->getActiveFunction()->getID()))
                                      : true) {
        loc->getType()->destroyValue(ctx, {loc}, ctx->getActiveFunction());
      }
    }
  }
}

} // namespace qat::IR