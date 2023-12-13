#include "./mix.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../generics.hpp"
#include "../qat_module.hpp"
#include "./reference.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include <algorithm>
#include <cmath>

namespace qat::IR {

#define TWO_POWER_4  16ULL
#define TWO_POWER_8  256ULL
#define TWO_POWER_16 65536ULL
#define TWO_POWER_32 4294967296ULL

MixType::MixType(Identifier _name, IR::OpaqueType* _opaquedTy, Vec<GenericParameter*> _generics, QatModule* _parent,
                 Vec<Pair<Identifier, Maybe<QatType*>>> _subtypes, Maybe<usize> _defaultVal, IR::Context* ctx,
                 bool _isPacked, const VisibilityInfo& _visibility, FileRange _fileRange, Maybe<MetaInfo> _metaInfo)
    : ExpandedType(std::move(_name), std::move(_generics), _parent, _visibility),
      EntityOverview("mixType", Json(), _name.range), subtypes(std::move(_subtypes)), isPack(_isPacked),
      defaultVal(_defaultVal), fileRange(std::move(_fileRange)), metaInfo(_metaInfo), opaquedType(_opaquedTy) {
  Maybe<String> foreignID;
  if (metaInfo) {
    foreignID = metaInfo->getForeignID();
  }
  for (const auto& sub : subtypes) {
    if (sub.second.has_value()) {
      auto* typ = sub.second.value();
      SHOW("Getting size of the subtype in SUM TYPE")
      usize size = typ->isOpaque()
                       ? ((typ->asOpaque()->hasSubType() && typ->asOpaque()->getSubType()->isTypeSized())
                              ? ctx->dataLayout->getTypeAllocSizeInBits(typ->asOpaque()->getSubType()->getLLVMType())
                              : typ->asOpaque()->getDeducedSize())
                       : ctx->dataLayout->getTypeAllocSizeInBits(typ->getLLVMType());
      SHOW("Got size " << size << " of subtype named " << sub.first.value)
      if (size > maxSize) {
        maxSize = size;
      }
    }
  }
  findTagBitWidth(subtypes.size());
  SHOW("Opaqued type is: " << opaquedType)
  SHOW("Tag bitwidth is " << tagBitWidth)
  linkingName = opaquedType->getNameForLinking();
  llvmType    = opaquedType->getLLVMType();
  llvm::cast<llvm::StructType>(llvmType)->setBody(
      {llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), llvm::Type::getIntNTy(ctx->llctx, maxSize)}, _isPacked);
  if (parent) {
    parent->mixTypes.push_back(this);
  }
  opaquedType->setSubType(this);
  ovInfo            = opaquedType->ovInfo;
  ovRange           = opaquedType->ovRange;
  ovMentions        = opaquedType->ovMentions;
  ovBroughtMentions = opaquedType->ovBroughtMentions;
}

LinkNames MixType::getLinkNames() const {
  Maybe<String> foreignID;
  if (metaInfo) {
    foreignID = metaInfo->getForeignID();
  }
  auto linkNames = parent->getLinkNames().newWith(LinkNameUnit(name.value, LinkUnitType::mix), foreignID);
  if (isGeneric()) {
    Vec<LinkNames> genericlinkNames;
    for (auto* param : generics) {
      if (param->isTyped()) {
        genericlinkNames.push_back(
            LinkNames({LinkNameUnit(param->asTyped()->getType()->getNameForLinking(), LinkUnitType::genericTypeValue)},
                      None, nullptr));
      } else if (param->isPrerun()) {
        auto* preRes = param->asPrerun();
        genericlinkNames.push_back(
            LinkNames({LinkNameUnit(preRes->getType()->toPrerunGenericString(preRes->getExpression()).value(),
                                    LinkUnitType::genericPrerunValue)},
                      None, nullptr));
      }
    }
    linkNames.addUnit(LinkNameUnit("", LinkUnitType::genericList, None, genericlinkNames), None);
  }
  return linkNames;
}

void MixType::updateOverview() {
  Vec<JsonValue> subTyJson;
  for (auto const& sub : subtypes) {
    subTyJson.push_back(Json()
                            ._("name", sub.first)
                            ._("hasType", sub.second.has_value())
                            ._("typeID", sub.second.has_value() ? sub.second.value()->getID() : JsonValue())
                            ._("type", sub.second.has_value() ? sub.second.value()->toString() : JsonValue()));
  }
  ovInfo._("typeID", getID())
      ._("fullName", getFullName())
      ._("moduleID", parent->getID())
      ._("tagBitWidth", tagBitWidth)
      ._("isPacked", isPack)
      ._("hasDefaultValue", defaultVal.has_value())
      ._("subTypes", subTyJson)
      ._("visibility", visibility);
  if (hasDefault()) {
    ovInfo._("defaultValue", defaultVal.value());
  }
}

void MixType::findTagBitWidth(usize typeCount) {
  tagBitWidth = 1;
  while (std::pow(2, tagBitWidth) <= typeCount) {
    tagBitWidth++;
  }
}

usize MixType::getIndexOfName(const String& name) const {
  for (usize i = 0; i < subtypes.size(); i++) {
    if (subtypes.at(i).first.value == name) {
      return i;
    }
  }
  // NOLINTNEXTLINE(clang-diagnostic-return-type)
}

bool MixType::hasDefault() const { return defaultVal.has_value(); }

usize MixType::getDefault() const { return defaultVal.value_or(0u); }

Pair<bool, bool> MixType::hasSubTypeWithName(const String& sname) const {
  for (const auto& sTy : subtypes) {
    if (sTy.first.value == sname) {
      return {true, sTy.second.has_value()};
    }
  }
  return {false, false};
}

QatType* MixType::getSubTypeWithName(const String& sname) const {
  for (const auto& sTy : subtypes) {
    if (sTy.first.value == sname) {
      return sTy.second.value();
    }
  }
  return nullptr;
}

void MixType::getMissingNames(Vec<Identifier>& vals, Vec<Identifier>& missing) const {
  for (const auto& sub : subtypes) {
    bool result = false;
    for (const auto& val : vals) {
      if (sub.first.value == val.value) {
        result = true;
        break;
      }
    }
    if (!result) {
      missing.push_back(sub.first);
    }
  }
}

usize MixType::getSubTypeCount() const { return subtypes.size(); }

bool MixType::isPacked() const { return isPack; }

usize MixType::getTagBitwidth() const { return tagBitWidth; }

u64 MixType::getDataBitwidth() const { return maxSize; }

FileRange MixType::getFileRange() const { return fileRange; }

bool MixType::isTypeSized() const { return true; }

bool MixType::isTriviallyCopyable() const { return false; }

bool MixType::isTriviallyMovable() const { return false; }

bool MixType::isCopyConstructible() const {
  for (auto sub : subtypes) {
    if (sub.second.has_value() && !sub.second.value()->isCopyConstructible()) {
      return false;
    }
  }
  return true;
}

bool MixType::isCopyAssignable() const {
  for (auto sub : subtypes) {
    if (sub.second.has_value() && !sub.second.value()->isCopyAssignable()) {
      return false;
    }
  }
  return true;
}

bool MixType::isMoveConstructible() const {
  for (auto sub : subtypes) {
    if (sub.second.has_value() && !sub.second.value()->isMoveConstructible()) {
      return false;
    }
  }
  return true;
}

bool MixType::isMoveAssignable() const {
  for (auto sub : subtypes) {
    if (sub.second.has_value() && !sub.second.value()->isMoveAssignable()) {
      return false;
    }
  }
  return true;
}

void MixType::copyConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isCopyConstructible()) {
    auto*      prevTag     = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                     ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
    auto*      prevDataPtr = ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u);
    auto*      resDataPtr  = ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u);
    IR::Block* trueBlock   = nullptr;
    IR::Block* falseBlock  = nullptr;
    IR::Block* restBlock   = new IR::Block(fun, fun->getBlock()->getParent());
    restBlock->linkPrevBlock(fun->getBlock());
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isCopyConstructible()) {
        auto* subTy = subtypes.at(i).second.value();
        trueBlock   = new IR::Block(fun, falseBlock ? falseBlock : fun->getBlock());
        falseBlock  = new IR::Block(fun, falseBlock ? falseBlock : fun->getBlock());
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(prevTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            trueBlock->getBB(), falseBlock->getBB());
        trueBlock->setActive(ctx->builder);
        subTy->copyConstructValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              resDataPtr, llvm::PointerType::get(subTy->getLLVMType(),
                                                                 ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreatePointerCast(
                              prevDataPtr, llvm::PointerType::get(subTy->getLLVMType(),
                                                                  ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
            fun);
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false),
                                 ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        falseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("Could not copy construct an instance of type " + ctx->highlightError(toString()), None);
  }
}

void MixType::moveConstructValue(IR::Context* ctx, IR::Value* first, IR::Value* second, IR::Function* fun) {
  if (isMoveConstructible()) {
    auto*      prevTag     = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                     ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 0u));
    auto*      prevDataPtr = ctx->builder.CreateStructGEP(getLLVMType(), second->getLLVM(), 1u);
    auto*      resDataPtr  = ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 1u);
    IR::Block* trueBlock   = nullptr;
    IR::Block* falseBlock  = nullptr;
    IR::Block* restBlock   = new IR::Block(fun, fun->getBlock()->getParent());
    restBlock->linkPrevBlock(fun->getBlock());
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isMoveConstructible()) {
        auto* subTy = subtypes.at(i).second.value();
        trueBlock   = new IR::Block(fun, falseBlock ? falseBlock : fun->getBlock());
        falseBlock  = new IR::Block(fun, falseBlock ? falseBlock : fun->getBlock());
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(prevTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            trueBlock->getBB(), falseBlock->getBB());
        trueBlock->setActive(ctx->builder);
        subTy->moveConstructValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              resDataPtr, llvm::PointerType::get(subTy->getLLVMType(),
                                                                 ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreatePointerCast(
                              prevDataPtr, llvm::PointerType::get(subTy->getLLVMType(),
                                                                  ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
            fun);
        ctx->builder.CreateStore(llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false),
                                 ctx->builder.CreateStructGEP(getLLVMType(), first->getLLVM(), 0u));
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        falseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("Could not move construct an instance of type " + ctx->highlightError(toString()), None);
  }
}

void MixType::copyAssignValue(IR::Context* ctx, IR::Value* firstInst, IR::Value* secondInst, IR::Function* fun) {
  if (isCopyAssignable()) {
    auto* firstTag          = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                      ctx->builder.CreateStructGEP(getLLVMType(), firstInst->getLLVM(), 0u));
    auto* firstData         = ctx->builder.CreateStructGEP(getLLVMType(), firstInst->getLLVM(), 1u);
    auto* secondTag         = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                      ctx->builder.CreateStructGEP(getLLVMType(), secondInst->getLLVM(), 0u));
    auto* secondData        = ctx->builder.CreateStructGEP(getLLVMType(), firstInst->getLLVM(), 1u);
    auto* sameTagTrueBlock  = new IR::Block(fun, fun->getBlock());
    auto* sameTagFalseBlock = new IR::Block(fun, fun->getBlock());
    auto* restBlock         = new IR::Block(fun, fun->getBlock()->getParent());
    restBlock->linkPrevBlock(fun->getBlock());
    ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(firstTag, secondTag), sameTagTrueBlock->getBB(),
                              sameTagFalseBlock->getBB());
    sameTagTrueBlock->setActive(ctx->builder);
    IR::Block* cmpTrueBlock  = nullptr;
    IR::Block* cmpFalseBlock = nullptr;
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isCopyAssignable()) {
        auto* subTy   = subtypes.at(i).second.value();
        cmpTrueBlock  = new IR::Block(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
        cmpFalseBlock = new IR::Block(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(firstTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            cmpTrueBlock->getBB(), cmpFalseBlock->getBB());
        cmpTrueBlock->setActive(ctx->builder);
        subTy->copyAssignValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              firstData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreatePointerCast(
                              secondData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                 ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        cmpFalseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    sameTagFalseBlock->setActive(ctx->builder);
    IR::Block* firstCmpTrueBlock  = nullptr;
    IR::Block* firstCmpFalseBlock = nullptr;
    IR::Block* firstCmpRestBlock  = new IR::Block(fun, sameTagFalseBlock);
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isDestructible()) {
        auto* subTy        = subtypes.at(i).second.value();
        firstCmpTrueBlock  = new IR::Block(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
        firstCmpFalseBlock = new IR::Block(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(firstTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            firstCmpTrueBlock->getBB(), firstCmpFalseBlock->getBB());
        firstCmpTrueBlock->setActive(ctx->builder);
        subTy->destroyValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              firstData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, firstCmpRestBlock->getBB());
        firstCmpFalseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, firstCmpRestBlock->getBB());
    firstCmpRestBlock->setActive(ctx->builder);
    IR::Block* secondCmpTrueBlock  = nullptr;
    IR::Block* secondCmpFalseBlock = nullptr;
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isCopyConstructible()) {
        auto* subTy         = subtypes.at(i).second.value();
        secondCmpTrueBlock  = new IR::Block(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
        secondCmpFalseBlock = new IR::Block(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(secondTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            secondCmpTrueBlock->getBB(), secondCmpFalseBlock->getBB());
        secondCmpTrueBlock->setActive(ctx->builder);
        subTy->copyConstructValue(
            ctx,

            new IR::Value(ctx->builder.CreatePointerCast(
                              firstData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreatePointerCast(
                              secondData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                 ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        secondCmpFalseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("Could not copy assign an instance of type " + ctx->highlightError(toString()), None);
  }
}

void MixType::moveAssignValue(IR::Context* ctx, IR::Value* firstInst, IR::Value* secondInst, IR::Function* fun) {
  if (isMoveAssignable()) {
    auto* firstTag          = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                      ctx->builder.CreateStructGEP(getLLVMType(), firstInst->getLLVM(), 0u));
    auto* firstData         = ctx->builder.CreateStructGEP(getLLVMType(), firstInst->getLLVM(), 1u);
    auto* secondTag         = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                      ctx->builder.CreateStructGEP(getLLVMType(), secondInst->getLLVM(), 0u));
    auto* secondData        = ctx->builder.CreateStructGEP(getLLVMType(), firstInst->getLLVM(), 1u);
    auto* sameTagTrueBlock  = new IR::Block(fun, fun->getBlock());
    auto* sameTagFalseBlock = new IR::Block(fun, fun->getBlock());
    auto* restBlock         = new IR::Block(fun, fun->getBlock()->getParent());
    restBlock->linkPrevBlock(fun->getBlock());
    ctx->builder.CreateCondBr(ctx->builder.CreateICmpEQ(firstTag, secondTag), sameTagTrueBlock->getBB(),
                              sameTagFalseBlock->getBB());
    sameTagTrueBlock->setActive(ctx->builder);
    IR::Block* cmpTrueBlock  = nullptr;
    IR::Block* cmpFalseBlock = nullptr;
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isCopyAssignable()) {
        auto* subTy   = subtypes.at(i).second.value();
        cmpTrueBlock  = new IR::Block(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
        cmpFalseBlock = new IR::Block(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(firstTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            cmpTrueBlock->getBB(), cmpFalseBlock->getBB());
        cmpTrueBlock->setActive(ctx->builder);
        subTy->moveAssignValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              firstData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreatePointerCast(
                              secondData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                 ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        cmpFalseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    sameTagFalseBlock->setActive(ctx->builder);
    IR::Block* firstCmpTrueBlock  = nullptr;
    IR::Block* firstCmpFalseBlock = nullptr;
    IR::Block* firstCmpRestBlock  = new IR::Block(fun, sameTagFalseBlock);
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isDestructible()) {
        auto* subTy        = subtypes.at(i).second.value();
        firstCmpTrueBlock  = new IR::Block(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
        firstCmpFalseBlock = new IR::Block(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(firstTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            firstCmpTrueBlock->getBB(), firstCmpFalseBlock->getBB());
        firstCmpTrueBlock->setActive(ctx->builder);
        subTy->destroyValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              firstData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, firstCmpRestBlock->getBB());
        firstCmpFalseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, firstCmpRestBlock->getBB());
    firstCmpRestBlock->setActive(ctx->builder);
    IR::Block* secondCmpTrueBlock  = nullptr;
    IR::Block* secondCmpFalseBlock = nullptr;
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isCopyConstructible()) {
        auto* subTy         = subtypes.at(i).second.value();
        secondCmpTrueBlock  = new IR::Block(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
        secondCmpFalseBlock = new IR::Block(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(secondTag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            secondCmpTrueBlock->getBB(), secondCmpFalseBlock->getBB());
        secondCmpTrueBlock->setActive(ctx->builder);
        subTy->moveConstructValue(
            ctx,

            new IR::Value(ctx->builder.CreatePointerCast(
                              firstData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            new IR::Value(ctx->builder.CreatePointerCast(
                              secondData, llvm::PointerType::get(subTy->getLLVMType(),
                                                                 ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        secondCmpFalseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("Could not move assign an instance of type " + ctx->highlightError(toString()), None);
  }
}

bool MixType::isDestructible() const {
  for (auto sub : subtypes) {
    if (sub.second.has_value() && !sub.second.value()->isDestructible()) {
      return false;
    }
  }
  return true;
}

void MixType::destroyValue(IR::Context* ctx, IR::Value* instance, IR::Function* fun) {
  if (isDestructible()) {
    auto*      tag        = ctx->builder.CreateLoad(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth),
                                                    ctx->builder.CreateStructGEP(getLLVMType(), instance->getLLVM(), 0u));
    auto*      dataPtr    = ctx->builder.CreateStructGEP(getLLVMType(), instance->getLLVM(), 1u);
    IR::Block* trueBlock  = nullptr;
    IR::Block* falseBlock = nullptr;
    IR::Block* restBlock  = new IR::Block(fun, fun->getBlock()->getParent());
    restBlock->linkPrevBlock(fun->getBlock());
    for (usize i = 0; i < subtypes.size(); i++) {
      if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->isDestructible()) {
        auto* subTy = subtypes.at(i).second.value();
        trueBlock   = new IR::Block(fun, falseBlock ? falseBlock : fun->getBlock());
        falseBlock  = new IR::Block(fun, falseBlock ? falseBlock : fun->getBlock());
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(tag,
                                      llvm::ConstantInt::get(llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), i, false)),
            trueBlock->getBB(), falseBlock->getBB());
        trueBlock->setActive(ctx->builder);
        subTy->destroyValue(
            ctx,
            new IR::Value(ctx->builder.CreatePointerCast(
                              dataPtr, llvm::PointerType::get(subTy->getLLVMType(),
                                                              ctx->dataLayout.value().getProgramAddressSpace())),
                          IR::ReferenceType::get(true, subTy, ctx), false, IR::Nature::temporary),
            fun);
        (void)IR::addBranch(ctx->builder, restBlock->getBB());
        falseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, restBlock->getBB());
    restBlock->setActive(ctx->builder);
  } else {
    ctx->Error("Could not destroy an instance of type " + ctx->highlightError(toString()), None);
  }
}

String MixType::toString() const { return getFullName(); }

TypeKind MixType::typeKind() const { return TypeKind::mixType; }

} // namespace qat::IR