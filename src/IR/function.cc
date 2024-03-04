#include "./function.hpp"
#include "../ast/function.hpp"
#include "../ast/types/generic_abstract.hpp"
#include "../show.hpp"
#include "./context.hpp"
#include "./control_flow.hpp"
#include "./generics.hpp"
#include "./logic.hpp"
#include "./qat_module.hpp"
#include "./types/function.hpp"
#include "./types/pointer.hpp"
#include "./types/region.hpp"
#include "link_names.hpp"
#include "meta_info.hpp"
#include "method.hpp"
#include "types/qat_type.hpp"
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

namespace qat::ir {

LocalValue::LocalValue(String _name, ir::Type* _type, bool _isVar, Function* fun, FileRange _fileRange)
    : Value(nullptr, _type, _isVar), EntityOverview("localValue",
                                                    Json()
                                                        ._("name", _name)
                                                        ._("typeID", _type->get_id())
                                                        ._("type", _type->to_string())
                                                        ._("is_variable", _isVar)
                                                        ._("functionID", fun->get_id()),
                                                    _fileRange),
      name(std::move(_name)), fileRange(std::move(_fileRange)) {
  SHOW("Type is " << type->to_string())
  SHOW("Creating llvm::AllocaInst for " << name)
  ll      = ir::Logic::newAlloca(fun, name, type->get_llvm_type());
  localID = utils::unique_id();
  SHOW("AllocaInst name is: " << ((llvm::AllocaInst*)ll)->getName().str());
}

String LocalValue::get_name() const { return name; }

llvm::AllocaInst* LocalValue::getAlloca() const { return (llvm::AllocaInst*)ll; }

FileRange LocalValue::get_file_range() const { return fileRange; }

ir::Value* LocalValue::to_new_ir_value() const {
  auto* result = ir::Value::get(get_llvm(), get_ir_type(), is_variable());
  result->set_local_id(get_local_id().value());
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
  name = (has_parent() ? (parent->get_name() + "_") : "") + std::to_string(index) + "b";
  bb   = llvm::BasicBlock::Create(fn->get_llvm_function()->getContext(), name, fn->get_llvm_function());
  SHOW("Created llvm::BasicBlock " << name)
}

String Block::get_name() const { return name; }

llvm::BasicBlock* Block::get_bb() const { return bb; }

bool Block::has_parent() const { return (parent != nullptr); }

bool Block::has_previous_block() const { return prevBlock != nullptr; }

Block* Block::get_previous_block() const { return prevBlock; }

bool Block::has_next_block() const { return nextBlock != nullptr; }

Block* Block::get_next_block() const { return nextBlock; }

void Block::link_previous_block(Block* block) {
  prevBlock        = block;
  block->nextBlock = this;
}

Block* Block::getParent() const { return parent; }

Function* Block::getFn() const { return fn; }

bool Block::hasValue(const String& name) const {
  SHOW("Number of local values: " << values.size())
  for (auto* val : values) {
    SHOW("Local value name is: " << val->get_name())
    SHOW("Local value type is: " << val->getType()->to_string())
    if (val->get_name() == name) {
      SHOW("Has local with name")
      return true;
    }
  }
  if (prevBlock) {
    if (prevBlock->hasValue(name)) {
      return true;
    }
  }
  if (has_parent()) {
    SHOW("Has parent block")
    return parent->hasValue(name);
  }
  SHOW("No local with name")
  return false;
}

LocalValue* Block::getValue(const String& name) const {
  for (auto* val : values) {
    if (val->get_name() == name) {
      return val;
    }
  }
  if (prevBlock && prevBlock->hasValue(name)) {
    return prevBlock->getValue(name);
  }
  if (has_parent()) {
    return parent->getValue(name);
  }
  return nullptr;
}

LocalValue* Block::new_value(const String& name, ir::Type* type, bool isVar, FileRange _fileRange) {
  values.push_back(LocalValue::get(name, type, isVar, fn, _fileRange));
  return values.back();
}

void Block::set_has_give() const { hasGive = true; }

bool Block::has_give_in_all_control_paths() const {
  if (isGhost) {
    return true;
  } else {
    if (hasGive) {
      return true;
    } else {
      if (!children.empty()) {
        bool result = true;
        for (auto* child : children) {
          if (!child->has_give_in_all_control_paths()) {
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

void Block::set_active(llvm::IRBuilder<>& builder) {
  SHOW("Setting active block: " << get_name())
  active = None;
  if (has_parent()) {
    parent->set_active(builder);
    parent->active = index;
  } else {
    fn->set_active_block(index);
  }
  builder.SetInsertPoint(bb);
}

void Block::collect_all_local_values_so_far(Vec<LocalValue*>& vals) const {
  if (has_parent()) {
    parent->collect_all_local_values_so_far(vals);
  }
  if (has_previous_block()) {
    prevBlock->collect_all_local_values_so_far(vals);
  }
  for (auto* val : values) {
    bool exists = false;
    for (auto* collected : vals) {
      if (collected->get_local_id() == val->get_local_id()) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      vals.push_back(val);
    }
  }
}

void Block::collect_locals_from(Vec<LocalValue*>& vals) const {
  for (auto* val : values) {
    vals.push_back(val);
  }
  for (auto* child : children) {
    child->collect_locals_from(vals);
  }
  if (nextBlock) {
    nextBlock->collect_locals_from(vals);
  }
}

Vec<LocalValue*>& Block::get_locals() { return values; }

bool Block::is_moved(const String& locID) const {
  if (prevBlock) {
    return prevBlock->is_moved(locID);
  }
  if (has_parent()) {
    auto parentRes = parent->is_moved(locID);
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

Maybe<FileRange> Block::get_file_range() const {
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

void Block::destroyLocals(ast::EmitCtx* ctx) {
  SHOW("Locals being destroyed for " << name)
  for (auto* loc : values) {
    if (loc->get_ir_type()->is_destructible()) {
      if (loc->get_ir_type()->is_pointer()
              ? (loc->get_ir_type()->as_pointer()->getOwner().is_parent_function() &&
                 (loc->get_ir_type()->as_pointer()->getOwner().owner_as_parent_function()->get_id() ==
                  ctx->get_fn()->get_id()))
              : true) {
        loc->get_ir_type()->destroy_value(ctx->irCtx, loc, ctx->get_fn());
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

Function::Function(Mod* _mod, Identifier _name, Maybe<LinkNames> _namingInfo, Vec<GenericParameter*> _generics,
                   ReturnType* returnType, Vec<Argument> _args, bool _isVariadicArguments, Maybe<FileRange> _fileRange,
                   const VisibilityInfo& _visibility_info, ir::Ctx* _ctx, bool isMemberFn,
                   Maybe<llvm::GlobalValue::LinkageTypes> llvmLinkage, Maybe<MetaInfo> _metaInfo)
    : Value(nullptr, nullptr, false), EntityOverview("function", Json(), _name.range), name(std::move(_name)),
      namingInfo(_namingInfo.value_or(LinkNames({}, None, _mod))), generics(std::move(_generics)), mod(_mod),
      arguments(std::move(_args)), visibility_info(_visibility_info), fileRange(std::move(_fileRange)),
      hasVariadicArguments(_isVariadicArguments), metaInfo(_metaInfo), ctx(_ctx) //
{
  Maybe<String> foreignID;
  Maybe<String> linkAlias;
  if (metaInfo) {
    foreignID = metaInfo->get_foreign_id();
    linkAlias = metaInfo->get_value_as_string_for(ir::MetaInfo::linkAsKey);
  }
  if (!foreignID.has_value()) {
    foreignID = get_module()->get_relevant_foreign_id();
  }
  if (!_namingInfo.has_value()) {
    namingInfo = mod->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::function), foreignID);
    if (isGeneric()) {
      Vec<LinkNames> genericLinkNames;
      for (auto* param : generics) {
        if (param->is_typed()) {
          genericLinkNames.push_back(LinkNames(
              {LinkNameUnit(param->as_typed()->get_type()->get_name_for_linking(), LinkUnitType::genericTypeValue)},
              None, nullptr));
        } else if (param->is_prerun()) {
          auto preRes = param->as_prerun()->get_expression();
          genericLinkNames.push_back(
              LinkNames({LinkNameUnit(preRes->get_ir_type()->to_prerun_generic_string(preRes).value(),
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
    argTypes.push_back(
        new ArgumentType(arg.get_name().value, arg.get_type(), arg.is_member_argument(), arg.get_variability()));
  }
  type = new FunctionType(returnType, argTypes, ctx->llctx);
  if (isMemberFn) {
    ll =
        llvm::Function::Create(llvm::cast<llvm::FunctionType>(get_ir_type()->get_llvm_type()),
                               llvmLinkage.value_or(DEFAULT_FUNCTION_LINKAGE), 0U, linkingName, mod->get_llvm_module());
  } else {
    ll =
        llvm::Function::Create(llvm::cast<llvm::FunctionType>(get_ir_type()->get_llvm_type()),
                               llvmLinkage.value_or(DEFAULT_FUNCTION_LINKAGE), 0U, linkingName, mod->get_llvm_module());
  }
  if (!is_method() && !isGeneric()) {
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

ir::Value* Function::call(ir::Ctx* irCtx, const Vec<llvm::Value*>& argValues, Maybe<String> localID, Mod* destMod) {
  SHOW("Linking function if it is external")
  auto* llvmFunction = llvm::cast<llvm::Function>(ll);
  if (destMod->get_id() != mod->get_id()) {
    // FIXME - This will prevent some functions with duplicate names in the global scope to be not linked during calls
    if (!destMod->get_llvm_module()->getFunction(llvmFunction->getName())) {
      llvm::Function::Create((llvm::FunctionType*)get_ir_type()->get_llvm_type(),
                             llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, llvmFunction->getAddressSpace(),
                             llvmFunction->getName(), destMod->get_llvm_module());
    }
  }
  SHOW("Getting return type")
  auto* retType = ((FunctionType*)get_ir_type())->get_return_type();
  SHOW("Getting function type")
  auto* fnTy = (llvm::FunctionType*)get_ir_type()->as_function()->get_llvm_type();
  SHOW("Got function type")
  SHOW("Creating LLVM call: " << linkingName << " with ID " << get_id())
  SHOW("Number of args: " << argValues.size())
  SHOW("Return type is " << retType->get_type()->to_string())
  auto result = ir::Value::get(irCtx->builder.CreateCall(fnTy, llvmFunction, argValues), retType->get_type(), false);
  if (get_module()->get_id() != destMod->get_id() && !get_module()->is_parent_mod_of(destMod)) {
    destMod->add_dependency(get_module());
  }
  if (localID && retType->is_return_self()) {
    result->set_local_id(localID.value());
  }
  return result;
}

Function* Function::Create(Mod* mod, Identifier name, Maybe<LinkNames> namingInfo, Vec<GenericParameter*> _generics,
                           ReturnType* returnTy, Vec<Argument> args, const bool hasVariadicArgs,
                           Maybe<FileRange> fileRange, const VisibilityInfo& visibilityInfo, ir::Ctx* irCtx,
                           Maybe<llvm::GlobalValue::LinkageTypes> linkage, Maybe<MetaInfo> metaInfo) {
  return new Function(mod, std::move(name), namingInfo, std::move(_generics), returnTy, std::move(args),
                      hasVariadicArgs, std::move(fileRange), visibilityInfo, irCtx, false, linkage, metaInfo);
}

void Function::update_overview() {
  Vec<JsonValue> localsJson;
  for (auto* block : blocks) {
    block->outputLocalOverview(localsJson);
  }
  ovInfo._("fullName", get_full_name())
      ._("functionID", get_id())
      ._("moduleID", mod->get_id())
      ._("visibility", visibility_info)
      ._("isVariadic", hasVariadicArguments)
      ._("locals", localsJson);
}

bool Function::has_variadic_args() const { return hasVariadicArguments; }

Identifier Function::arg_name_at(u32 index) const { return arguments.at(index).get_name(); }

Identifier Function::get_name() const { return name; }

ir::Mod* Function::get_module() const { return mod; }

bool Function::has_generic_parameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->get_name().value == name) {
      return true;
    }
  }
  return false;
}

String Function::get_random_alloca_name() const {
  localNameCounter++;
  return std::to_string(localNameCounter) + "_new";
}

GenericParameter* Function::get_generic_parameter(const String& name) const {
  for (auto* gen : generics) {
    if (gen->get_name().value == name) {
      return gen;
    }
  }
  return nullptr;
}

String Function::get_full_name() const { return mod->get_fullname_with_child(name.value); }

bool Function::is_accessible(const AccessInfo& req_info) const {
  return Visibility::is_accessible(visibility_info, req_info);
}

ir::LocalValue* Function::get_function_common_index() {
  if (!strComparisonIndex) {
    strComparisonIndex = get_first_block()->new_value("qat'function'commonIndex",
                                                      // NOLINTNEXTLINE(readability-magic-numbers)
                                                      ir::UnsignedType::get(64u, ctx), true, name.range);
  }
  return strComparisonIndex;
}

VisibilityInfo Function::get_visibility() const { return visibility_info; }

bool Function::is_method() const { return false; }

llvm::Function* Function::get_llvm_function() { return (llvm::Function*)ll; }

void Function::set_active_block(usize index) const { activeBlock = index; }

Block* Function::get_block() const { return blocks.at(activeBlock)->getActive(); }

Block* Function::get_first_block() const { return blocks.at(0); };

usize Function::get_block_count() const { return blocks.size(); }

usize& Function::get_copied_counter() { return copiedCounter; }

usize& Function::get_moved_counter() { return movedCounter; }

GenericFunction::GenericFunction(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                 Maybe<ast::PrerunExpression*> _constraint, ast::FunctionPrototype* _functionDef,
                                 Mod* _parent, const VisibilityInfo& _visibInfo)
    : EntityOverview("genericFunction",
                     Json()
                         ._("fullName", _parent->get_fullname_with_child(_name.value))
                         ._("moduleID", _parent->get_id())
                         ._("visibility", _visibInfo),
                     _name.range),
      name(std::move(_name)), generics(std::move(_generics)), functionDefinition(_functionDef), constraint(_constraint),
      parent(_parent), visibInfo(_visibInfo) {
  parent->genericFunctions.push_back(this);
}

Identifier GenericFunction::get_name() const { return name; }

VisibilityInfo GenericFunction::get_visibility() const { return visibInfo; }

usize GenericFunction::getTypeCount() const { return generics.size(); }

usize GenericFunction::getVariantCount() const { return variants.size(); }

Mod* GenericFunction::get_module() const { return parent; }

ast::GenericAbstractType* GenericFunction::getGenericAt(usize index) const { return generics.at(index); }

useit bool GenericFunction::all_generics_have_default() const {
  for (auto* gen : generics) {
    if (!gen->hasDefault()) {
      return false;
    }
  }
  return true;
}

Function* GenericFunction::fill_generics(Vec<ir::GenericToFill*> types, ir::Ctx* irCtx, const FileRange& fileRange) {
  for (auto var : variants) {
    if (var.check(
            irCtx, [&](const String& msg, const FileRange& rng) { irCtx->Error(msg, rng); }, types)) {
      return var.get();
    }
  }
  ir::fill_generics(irCtx, generics, types, fileRange);
  if (constraint.has_value()) {
    auto checkVal = constraint.value()->emit(ast::EmitCtx::get(irCtx, parent));
    if (checkVal->get_ir_type()->is_bool()) {
      if (!llvm::cast<llvm::ConstantInt>(checkVal->get_llvm_constant())->getValue().getBoolValue()) {
        irCtx->Error("The provided generic parameters for the generic function do not satisfy the constraints",
                     fileRange,
                     Pair<String, FileRange>{"The constraint can be found here", constraint.value()->fileRange});
      }
    } else {
      irCtx->Error("The constraints for generic parameters should be of " + irCtx->color("bool") +
                       " type. Got an expression of " + irCtx->color(checkVal->get_ir_type()->to_string()),
                   constraint.value()->fileRange);
    }
  }
  auto variantName = ir::Logic::get_generic_variant_name(name.value, types);
  functionDefinition->set_variant_name(variantName);
  Vec<ir::GenericParameter*> genParams;
  for (auto genAb : generics) {
    genParams.push_back(genAb->toIRGenericType());
  }
  auto prevTemp = irCtx->allActiveGenerics;
  irCtx->add_active_generic(
      ir::GenericEntityMarker{variantName, ir::GenericEntityType::function, fileRange, 0, genParams}, true);
  auto* fun                    = functionDefinition->create_function(parent, irCtx);
  functionDefinition->function = fun;
  functionDefinition->emit_definition(parent, irCtx);
  variants.push_back(GenericVariant<Function>(fun, types));
  for (auto* temp : generics) {
    temp->unset();
  }
  functionDefinition->unset_variant_name();
  if (irCtx->get_active_generic().warningCount > 0) {
    auto count = irCtx->get_active_generic().warningCount;
    irCtx->remove_active_generic();
    irCtx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                       " generated while creating generic function: " + irCtx->highlightWarning(variantName),
                   fileRange);
  } else {
    irCtx->remove_active_generic();
  }
  return fun;
}

void destructor_caller(ir::Ctx* irCtx, ir::Function* fun) {
  SHOW("DestructorCaller")
  Vec<ir::LocalValue*> locals;
  fun->get_block()->collect_all_local_values_so_far(locals);
  SHOW(locals.size() << " locals collected so far")
  for (auto* loc : locals) {
    SHOW("Local name is: " << loc->get_name() << " and type is " << loc->getType()->to_string())
    if (loc->is_reference()) {
      continue;
    }
    if (loc->get_ir_type()->is_expanded() && loc->get_ir_type()->as_expanded()->has_destructor()) {
      auto* eTy        = loc->get_ir_type()->as_expanded();
      auto* destructor = eTy->get_destructor();
      (void)destructor->call(irCtx, {loc->getAlloca()}, None, fun->get_module());
    } else if (loc->get_ir_type()->is_destructible()) {
      loc->get_ir_type()->destroy_value(irCtx, loc->to_new_ir_value(), fun);
      SHOW("Destroyed value using type level feature")
    } else if (loc->get_ir_type()->is_pointer() && loc->get_ir_type()->as_pointer()->getOwner().is_parent_function() &&
               loc->get_ir_type()->as_pointer()->getOwner().owner_as_parent_function()->get_id() == fun->get_id()) {
      auto* ptrTy = loc->get_ir_type()->as_pointer();
      if (ptrTy->get_subtype()->is_struct() && ptrTy->get_subtype()->as_struct()->has_destructor()) {
        auto* dstrFn = ptrTy->get_subtype()->as_struct()->get_destructor();
        if (ptrTy->isMulti()) {
          auto* currBlock = fun->get_block();
          auto* condBlock = new ir::Block(fun, currBlock);
          auto* trueBlock = new ir::Block(fun, currBlock);
          auto* restBlock = new ir::Block(fun, nullptr);
          restBlock->link_previous_block(currBlock);
          // NOLINTNEXTLINE(readability-magic-numbers)
          auto* count = currBlock->new_value(utils::unique_id(), ir::UnsignedType::get(64u, irCtx), true, {""});
          irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u, false),
                                     count->get_llvm());
          irCtx->builder.CreateCondBr(
              irCtx->builder.CreateICmpNE(
                  irCtx->builder.CreatePtrDiff(
                      ptrTy->get_subtype()->get_llvm_type(),
                      irCtx->builder.CreateLoad(
                          ptrTy->get_subtype()->get_llvm_type()->getPointerTo(),
                          irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), loc->get_llvm(), 0u)),
                      llvm::ConstantPointerNull::get(ptrTy->get_subtype()->get_llvm_type()->getPointerTo())),
                  llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u)),
              condBlock->get_bb(), restBlock->get_bb());
          condBlock->set_active(irCtx->builder);
          SHOW("Set condition block active")
          irCtx->builder.CreateCondBr(
              irCtx->builder.CreateICmpULT(
                  irCtx->builder.CreateLoad(count->get_ir_type()->get_llvm_type(), count->get_llvm()),
                  irCtx->builder.CreateLoad(
                      count->get_ir_type()->get_llvm_type(),
                      irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), loc->get_llvm(), 1u))),
              trueBlock->get_bb(), restBlock->get_bb());
          trueBlock->set_active(irCtx->builder);
          SHOW("Set trueblock active")
          (void)dstrFn->call(
              irCtx,
              {irCtx->builder.CreateLoad(ptrTy->get_subtype()->get_llvm_type()->getPointerTo(),
                                         irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), loc->get_llvm(), 0u))},
              None, fun->get_module());
          irCtx->builder.CreateStore(
              irCtx->builder.CreateAdd(
                  llvm::ConstantInt::get(count->get_ir_type()->get_llvm_type(), 1u, false),
                  irCtx->builder.CreateLoad(count->get_ir_type()->get_llvm_type(), count->get_llvm())),
              count->get_llvm());
          (void)ir::add_branch(irCtx->builder, condBlock->get_bb());
          restBlock->set_active(irCtx->builder);
        } else {
          auto* currBlock = fun->get_block();
          auto* trueBlock = new ir::Block(fun, currBlock);
          auto* restBlock = new ir::Block(fun, nullptr);
          restBlock->link_previous_block(currBlock);
          irCtx->builder.CreateCondBr(
              irCtx->builder.CreateICmpNE(
                  irCtx->builder.CreatePtrDiff(
                      ptrTy->get_llvm_type(), irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), loc->get_llvm()),
                      llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(ptrTy->get_llvm_type()))),
                  llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u)),
              trueBlock->get_bb(), restBlock->get_bb());
          trueBlock->set_active(irCtx->builder);
          irCtx->builder.CreateCall(dstrFn->get_llvm_function()->getFunctionType(), dstrFn->get_llvm_function(),
                                    {irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), loc->get_llvm())});
          (void)ir::add_branch(irCtx->builder, restBlock->get_bb());
          restBlock->set_active(irCtx->builder);
        }
      }
      fun->get_module()->link_native(NativeUnit::free);
      auto* freeFn = fun->get_module()->get_llvm_module()->getFunction("free");
      irCtx->builder.CreateCall(
          freeFn->getFunctionType(), freeFn,
          {ptrTy->isMulti()
               ? irCtx->builder.CreatePointerCast(
                     irCtx->builder.CreateLoad(
                         ptrTy->get_subtype()->get_llvm_type()->getPointerTo(),
                         irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), loc->get_llvm(), 0u)),
                     llvm::Type::getInt8Ty(irCtx->llctx)->getPointerTo())
               : irCtx->builder.CreatePointerCast(irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), loc->get_llvm()),
                                                  llvm::Type::getInt8Ty(irCtx->llctx)->getPointerTo())});
    }
  }
  locals.clear();
}

void method_handler(ir::Ctx* irCtx, ir::Function* fun) {
  if (fun->is_method()) {
    auto* mFn = (ir::Method*)fun;
    // FIXME - Change this
    if (!mFn->get_parent_type()->is_struct()) {
      return;
    }
    auto* cTy = mFn->get_parent_type()->as_struct();
    if (mFn->get_method_type() == MethodType::destructor) {
      for (usize i = 0; i < cTy->get_members().size(); i++) {
        auto& mem = cTy->get_members().at(i);
        if (mem->type->is_pointer() && mem->type->as_pointer()->getOwner().isType() &&
            (mem->type->as_pointer()->getOwner().ownerAsType()->get_id() == mem->type->get_id())) {
          auto* ptrTy  = mem->type->as_pointer();
          auto* memPtr = irCtx->builder.CreateStructGEP(
              ptrTy->get_llvm_type(),
              irCtx->builder.CreateStructGEP(cTy->get_llvm_type(), mFn->get_first_block()->getValue("''")->get_llvm(),
                                             i),
              0u);
          if (ptrTy->get_subtype()->is_struct() && ptrTy->get_subtype()->as_struct()->has_destructor()) {
            auto* dstrFn = ptrTy->get_subtype()->as_struct()->get_destructor();
            if (ptrTy->isMulti()) {
              auto* currBlock = fun->get_block();
              auto* condBlock = new ir::Block(fun, currBlock);
              auto* trueBlock = new ir::Block(fun, currBlock);
              auto* restBlock = new ir::Block(fun, nullptr);
              restBlock->link_previous_block(currBlock);
              // NOLINTNEXTLINE(readability-magic-numbers)
              auto* count = currBlock->new_value(utils::unique_id(), ir::UnsignedType::get(64u, irCtx), true, {""});
              irCtx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u, false),
                                         count->get_llvm());
              irCtx->builder.CreateCondBr(
                  irCtx->builder.CreateICmpNE(
                      irCtx->builder.CreatePtrDiff(
                          ptrTy->get_subtype()->get_llvm_type(),
                          irCtx->builder.CreateLoad(ptrTy->get_subtype()->get_llvm_type()->getPointerTo(),
                                                    irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), memPtr, 0u)),
                          llvm::ConstantPointerNull::get(ptrTy->get_subtype()->get_llvm_type()->getPointerTo())),
                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u)),
                  condBlock->get_bb(), restBlock->get_bb());
              condBlock->set_active(irCtx->builder);
              SHOW("Set condition block active")
              irCtx->builder.CreateCondBr(
                  irCtx->builder.CreateICmpULT(
                      irCtx->builder.CreateLoad(count->get_ir_type()->get_llvm_type(), count->get_llvm()),
                      irCtx->builder.CreateLoad(count->get_ir_type()->get_llvm_type(),
                                                irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), memPtr, 1u))),
                  trueBlock->get_bb(), restBlock->get_bb());
              trueBlock->set_active(irCtx->builder);
              SHOW("Set trueblock active")
              (void)dstrFn->call(
                  irCtx,
                  {irCtx->builder.CreateLoad(ptrTy->get_subtype()->get_llvm_type()->getPointerTo(),
                                             irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), memPtr, 0u))},
                  None, fun->get_module());
              irCtx->builder.CreateStore(
                  irCtx->builder.CreateAdd(
                      llvm::ConstantInt::get(count->get_ir_type()->get_llvm_type(), 1u, false),
                      irCtx->builder.CreateLoad(count->get_ir_type()->get_llvm_type(), count->get_llvm())),
                  count->get_llvm());
              (void)ir::add_branch(irCtx->builder, condBlock->get_bb());
              restBlock->set_active(irCtx->builder);
            } else {
              auto* currBlock = fun->get_block();
              auto* trueBlock = new ir::Block(fun, currBlock);
              auto* restBlock = new ir::Block(fun, nullptr);
              restBlock->link_previous_block(currBlock);
              irCtx->builder.CreateCondBr(
                  irCtx->builder.CreateICmpNE(
                      irCtx->builder.CreatePtrDiff(
                          ptrTy->get_llvm_type(), irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), memPtr),
                          llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(ptrTy->get_llvm_type()))),
                      llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), 0u)),
                  trueBlock->get_bb(), restBlock->get_bb());
              trueBlock->set_active(irCtx->builder);
              irCtx->builder.CreateCall(dstrFn->get_llvm_function()->getFunctionType(), dstrFn->get_llvm_function(),
                                        {irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), memPtr)});
              (void)ir::add_branch(irCtx->builder, restBlock->get_bb());
              restBlock->set_active(irCtx->builder);
            }
          }
          fun->get_module()->link_native(NativeUnit::free);
          auto* freeFn = fun->get_module()->get_llvm_module()->getFunction("free");
          irCtx->builder.CreateCall(
              freeFn->getFunctionType(), freeFn,
              {ptrTy->isMulti()
                   ? irCtx->builder.CreatePointerCast(
                         irCtx->builder.CreateLoad(ptrTy->get_subtype()->get_llvm_type()->getPointerTo(),
                                                   irCtx->builder.CreateStructGEP(ptrTy->get_llvm_type(), memPtr, 0u)),
                         llvm::Type::getInt8Ty(irCtx->llctx)->getPointerTo())
                   : irCtx->builder.CreatePointerCast(irCtx->builder.CreateLoad(ptrTy->get_llvm_type(), memPtr),
                                                      llvm::Type::getInt8Ty(irCtx->llctx)->getPointerTo())});
        }
      }
      if (fun->get_block_count() >= 1 && fun->get_first_block()->hasValue("''")) {
        SHOW("Destructor self value is zero assigned")
        auto* selfVal = fun->get_first_block()->getValue("''");
        irCtx->builder.CreateStore(llvm::Constant::getNullValue(cTy->get_llvm_type()), selfVal->getAlloca());
      } else {
        SHOW("Destructor has no self value")
      }
    }
  }
}

void function_return_handler(ir::Ctx* irCtx, ir::Function* fun, const FileRange& fileRange) {
  SHOW("Starting function return handle for: " << fun->get_full_name())
  // FIXME - Support destructors for types besides core types
  auto* block = fun->get_block();
  auto* retTy = fun->get_ir_type()->as_function()->get_return_type();
  if (block->get_bb()->empty()) {
    SHOW("Empty instruction list in block")
    if (retTy->get_type()->is_void()) {
      SHOW("Calling destructor caller")
      destructor_caller(irCtx, fun);
      SHOW("Calling member function caller")
      method_handler(irCtx, fun);
      if (fun->get_full_name() == "main") {
        for (auto* reg : Type::allRegions()) {
          reg->destroyObjects(irCtx);
        }
      }
      irCtx->builder.CreateRetVoid();
    } else {
      irCtx->Error("Missing given value in all control paths", fileRange);
    }
  } else {
    auto* lastInst = ((llvm::Instruction*)&block->get_bb()->back());
    if (!llvm::isa<llvm::ReturnInst>(lastInst)) {
      SHOW("Last instruction is not a return")
      if (retTy->get_type()->is_void()) {
        SHOW("Calling destructor caller")
        destructor_caller(irCtx, fun);
        SHOW("Calling member function handler")
        method_handler(irCtx, fun);
        if (fun->get_full_name() == "main") {
          for (auto* reg : Type::allRegions()) {
            reg->destroyObjects(irCtx);
          }
        }
        irCtx->builder.CreateRetVoid();
      } else {
        irCtx->Error("Missing given value in all control paths", fileRange);
      }
    }
  }
}

void destroy_locals_from(ir::Ctx* irCtx, ir::Block* block) {
  Vec<ir::LocalValue*> locals;
  block->collect_locals_from(locals);
  for (auto* loc : locals) {
    if (loc->get_ir_type()->is_destructible()) {
      if (loc->get_ir_type()->is_pointer()
              ? (loc->get_ir_type()->as_pointer()->getOwner().is_parent_function() &&
                 (loc->get_ir_type()->as_pointer()->getOwner().owner_as_parent_function()->get_id() ==
                  block->getFn()->get_id()))
              : true) {
        loc->get_ir_type()->destroy_value(irCtx, loc, block->getFn());
      }
    }
  }
}

} // namespace qat::ir