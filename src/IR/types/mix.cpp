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
  ovInfo               = opaquedType->ovInfo;
  ovRange              = opaquedType->ovRange;
  ovMentions           = opaquedType->ovMentions;
  ovBroughtMentions    = opaquedType->ovBroughtMentions;
  auto needsDestructor = false;
  for (auto sub : subtypes) {
    if (sub.second.has_value()) {
      SHOW("Mix type subfield type is: " << sub.second.value()->toString())
      if (sub.second.value()->isDestructible()) {
        SHOW("Setting needs destructor")
        // FIXME - Problem is order of creation of types & destructors
        needsDestructor = true;
        break;
      }
    }
  }
  SHOW("Mix type needs destructor: " << (needsDestructor ? "true" : "false"))
  // NOTE - Possibly make this modular when mix types can have custom destructors
  if (needsDestructor) {
    destructor           = IR::MemberFunction::CreateDestructor(this, nullptr, _name.range, _fileRange, ctx);
    auto* entryBlock     = new IR::Block(destructor.value(), nullptr);
    auto* tagIntTy       = llvm::Type::getIntNTy(ctx->llctx, tagBitWidth);
    auto* remainingBlock = new IR::Block(destructor.value(), nullptr);
    remainingBlock->linkPrevBlock(entryBlock);
    entryBlock->setActive(ctx->builder);
    auto* inst = destructor.value()->getLLVMFunction()->getArg(0);
    for (auto sub : subtypes) {
      if (sub.second.has_value() && sub.second.value()->isDestructible()) {
        auto* subTy = sub.second.value();
        SHOW("Getting index of name of subfield")
        auto subIdx = getIndexOfName(sub.first.value);
        SHOW("Getting current block")
        auto* currBlock  = destructor.value()->getBlock();
        auto* trueBlock  = new IR::Block(destructor.value(), currBlock);
        auto* falseBlock = new IR::Block(destructor.value(), currBlock);
        SHOW("Created true and false blocks")
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(
                ctx->builder.CreateLoad(tagIntTy, ctx->builder.CreateStructGEP(llvmType, inst, 0u)),
                llvm::ConstantInt::get(tagIntTy, subIdx)),
            trueBlock->getBB(), falseBlock->getBB());
        trueBlock->setActive(ctx->builder);
        subTy->destroyValue(
            ctx,
            {new IR::Value(ctx->builder.CreatePointerCast(
                               ctx->builder.CreateStructGEP(llvmType, inst, 1u),
                               llvm::PointerType::get(subTy->getLLVMType(), ctx->dataLayout->getProgramAddressSpace())),
                           IR::ReferenceType::get(false, subTy, ctx), false, IR::Nature::temporary)},
            destructor.value());
        (void)IR::addBranch(ctx->builder, remainingBlock->getBB());
        falseBlock->setActive(ctx->builder);
      }
    }
    (void)IR::addBranch(ctx->builder, remainingBlock->getBB());
    remainingBlock->setActive(ctx->builder);
    ctx->builder.CreateStore(llvm::ConstantExpr::getNullValue(llvmType), inst);
    ctx->builder.CreateRetVoid();
  }
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

bool MixType::isTriviallyCopyable() const {
  if (explicitTrivialCopy) {
    return true;
  } else if (hasCopyConstructor() || hasCopyAssignment()) {
    return false;
  } else {
    auto result = true;
    for (auto sub : subtypes) {
      if (sub.second.has_value()) {
        if (!sub.second.value()->isTriviallyCopyable()) {
          result = false;
          break;
        }
      }
    }
    return result;
  }
}

bool MixType::isTriviallyMovable() const {
  if (explicitTrivialMove) {
    return true;
  } else if (hasMoveConstructor() || hasMoveAssignment()) {
    return false;
  } else {
    for (auto sub : subtypes) {
      if (sub.second.has_value()) {
        if (!sub.second.value()->isTriviallyMovable()) {
          return false;
        }
      }
    }
    return true;
  }
}

String MixType::toString() const { return getFullName(); }

TypeKind MixType::typeKind() const { return TypeKind::mixType; }

} // namespace qat::IR