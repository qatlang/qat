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

namespace qat::IR {

#define TWO_POWER_4  16ULL
#define TWO_POWER_8  256ULL
#define TWO_POWER_16 65536ULL
#define TWO_POWER_32 4294967296ULL

MixType::MixType(Identifier _name, Vec<GenericType*> _generics, QatModule* _parent,
                 Vec<Pair<Identifier, Maybe<QatType*>>> _subtypes, Maybe<usize> _defaultVal, IR::Context* ctx,
                 bool _isPacked, const utils::VisibilityInfo& _visibility, FileRange _fileRange)
    : ExpandedType(std::move(_name), std::move(_generics), _parent, _visibility),
      EntityOverview("mixType",
                     Json()
                         ._("moduleID", _parent->getID())
                         ._("hasDefaultValue", _defaultVal.has_value())
                         ._("visibility", _visibility),
                     _name.range),
      subtypes(std::move(_subtypes)), isPack(_isPacked), defaultVal(_defaultVal), fileRange(std::move(_fileRange)) {
  for (const auto& sub : subtypes) {
    if (sub.second.has_value()) {
      auto* typ = sub.second.value();
      SHOW("Getting size of the subtype in SUM TYPE")
      usize size = parent->getLLVMModule()->getDataLayout().getTypeAllocSizeInBits(typ->getLLVMType());
      SHOW("Got size " << size << " of subtype named " << sub.first.value)
      if (size > maxSize) {
        maxSize = size;
      }
    }
  }
  findTagBitWidth(subtypes.size());
  SHOW("Tag bitwidth is " << tagBitWidth)
  llvmType = llvm::StructType::create(
      ctx->llctx, {llvm::Type::getIntNTy(ctx->llctx, tagBitWidth), llvm::Type::getIntNTy(ctx->llctx, maxSize)},
      parent->getFullNameWithChild(name.value), _isPacked);
  if (parent) {
    parent->mixTypes.push_back(this);
  }
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
    destructor           = IR::MemberFunction::CreateDestructor(this, _name.range, _fileRange, ctx->llctx);
    auto* entryBlock     = new IR::Block(destructor, nullptr);
    auto* tagIntTy       = llvm::Type::getIntNTy(ctx->llctx, tagBitWidth);
    auto* remainingBlock = new IR::Block(destructor, nullptr);
    remainingBlock->linkPrevBlock(entryBlock);
    entryBlock->setActive(ctx->builder);
    auto* inst = destructor->getLLVMFunction()->getArg(0);
    for (auto sub : subtypes) {
      if (sub.second.has_value() && sub.second.value()->isDestructible()) {
        auto* subTy = sub.second.value();
        SHOW("Getting index of name of subfield")
        auto subIdx = getIndexOfName(sub.first.value);
        SHOW("Getting current block")
        auto* currBlock  = destructor->getBlock();
        auto* trueBlock  = new IR::Block(destructor, currBlock);
        auto* falseBlock = new IR::Block(destructor, currBlock);
        SHOW("Created true and false blocks")
        ctx->builder.CreateCondBr(
            ctx->builder.CreateICmpEQ(
                ctx->builder.CreateLoad(tagIntTy, ctx->builder.CreateStructGEP(llvmType, inst, 0u)),
                llvm::ConstantInt::get(tagIntTy, subIdx)),
            trueBlock->getBB(), falseBlock->getBB());
        trueBlock->setActive(ctx->builder);
        subTy->destroyValue(
            ctx,
            {new IR::Value(ctx->builder.CreatePointerCast(ctx->builder.CreateStructGEP(llvmType, inst, 1u),
                                                          llvm::PointerType::get(subTy->getLLVMType(), 0u)),
                           IR::ReferenceType::get(false, subTy, ctx->llctx), false, IR::Nature::temporary)},
            destructor);
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
      ._("tagBitWidth", (unsigned long long)tagBitWidth)
      ._("isPacked", isPack)
      ._("subTypes", subTyJson);
  if (hasDefault()) {
    ovInfo._("defaultValue", (unsigned long long)defaultVal.value());
  }
}

void MixType::findTagBitWidth(usize typeCount) {
  auto calc = 2;
  while (calc <= typeCount) {
    calc <<= 1;
    tagBitWidth++;
  }
}

usize MixType::getIndexOfName(const String& name) const {
  for (usize i = 0; i < subtypes.size(); i++) {
    if (subtypes.at(i).first.value == name) {
      SHOW("Got index")
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

String MixType::toString() const { return getFullName(); }

TypeKind MixType::typeKind() const { return TypeKind::mixType; }

} // namespace qat::IR