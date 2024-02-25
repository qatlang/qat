#include "./define_mix_type.hpp"
#include "../IR/logic.hpp"
#include <cmath>

namespace qat::ast {

void DefineMixType::create_entity(IR::QatModule* mod, IR::Context* ctx) {
  SHOW("CreateEntity: " << name.value)
  mod->entity_name_check(ctx, name, IR::EntityType::mixType);
  entityState = mod->add_entity(name, IR::EntityType::mixType, this, IR::EmitPhase::phase_2);
}

void DefineMixType::update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) {
  for (auto& sub : subtypes) {
    if (sub.second.has_value()) {
      sub.second.value()->update_dependencies(IR::EmitPhase::phase_2, IR::DependType::complete, entityState, ctx);
    }
  }
}

void DefineMixType::create_opaque(IR::QatModule* mod, IR::Context* ctx) {
  usize maxSubtypeSize          = 8u;
  bool  foundSizeForAssociation = true;
  usize tagBitwidth             = 1u;
  while (std::pow(2, tagBitwidth) < (subtypes.size() + 1)) {
    tagBitwidth++;
  }
  for (auto& subty : subtypes) {
    if (subty.second.has_value()) {
      auto subTySize = subty.second.value()->getTypeSizeInBits(ctx);
      if (subTySize.has_value()) {
        if (maxSubtypeSize < subTySize.value()) {
          maxSubtypeSize = subTySize.value();
        }
      } else {
        foundSizeForAssociation = false;
        break;
      }
    }
  }
  opaquedType = IR::OpaqueType::get(
      name, {}, None, IR::OpaqueSubtypeKind::mix, mod,
      foundSizeForAssociation
          ? Maybe<usize>(ctx->dataLayout->getTypeAllocSizeInBits(llvm::StructType::get(
                ctx->llctx,
                {llvm::Type::getIntNTy(ctx->llctx, tagBitwidth), llvm::Type::getIntNTy(ctx->llctx, maxSubtypeSize)},
                isPacked)))
          : None,
      ctx->getVisibInfo(visibSpec), ctx->llctx, None);
}

void DefineMixType::do_phase(IR::EmitPhase phase, IR::QatModule* mod, IR::Context* ctx) {
  if (phase == IR::EmitPhase::phase_1) {
    create_opaque(mod, ctx);
  } else if (phase == IR::EmitPhase::phase_2) {
    create_type(mod, ctx);
  }
}

void DefineMixType::create_type(IR::QatModule* mod, IR::Context* ctx) {
  if (opaquedType) {
    ctx->setActiveType(opaquedType);
  }
  Vec<Pair<Identifier, Maybe<IR::QatType*>>> subTypesIR;
  bool                                       hasAssociatedType = false;
  for (usize i = 0; i < subtypes.size(); i++) {
    for (usize j = i + 1; j < subtypes.size(); j++) {
      if (subtypes.at(i).first.value == subtypes.at(j).first.value) {
        ctx->Error("The name of the subtype of the mix is repeating here. Please "
                   "check logic & make necessary changes",
                   fRanges.at(j));
      }
    }
    if (!hasAssociatedType && subtypes.at(i).second.has_value()) {
      hasAssociatedType = true;
    }
    if (defaultVal.has_value()) {
      if (defaultVal.value() == i) {
        if (subtypes.at(i).second.has_value()) {
          ctx->Error("A subfield with a value associated with them cannot be "
                     "used as a default value",
                     FileRange{fRanges.at(i), subtypes.at(i).second.value()->fileRange});
        }
      }
    }
    auto subTypeTy = subtypes.at(i).second.has_value() ? subtypes.at(i).second.value()->emit(ctx) : nullptr;
    if (subtypes.at(i).second.has_value() && subTypeTy->isOpaque() && !subTypeTy->asOpaque()->isTypeSized()) {
      if (opaquedType && subTypeTy->asOpaque()->isSame(opaquedType)) {
        ctx->Error(
            "Type nesting found. The variant " + ctx->highlightError(subtypes.at(i).first.value) + " of mix type " +
                ctx->highlightError(opaquedType->toString()) +
                " has the same type associated with it as its parent. Check the code for mistakes or use a pointer or reference to the parent type as the variant type",
            subtypes.at(i).second.value()->fileRange);
      } else {
        ctx->Error("The variant " + ctx->highlightError(subtypes.at(i).first.value) + " of mix type " +
                       ctx->highlightError(opaquedType->toString()) +
                       " has an incomplete type with an unknown size associated with it",
                   subtypes.at(i).second.value()->fileRange);
      }
    }
    subTypesIR.push_back(Pair<Identifier, Maybe<IR::QatType*>>(
        subtypes.at(i).first, subtypes.at(i).second.has_value() ? Maybe<IR::QatType*>(subTypeTy) : None));
  }
  if (!hasAssociatedType) {
    ctx->Error("No types associated to any of the subfields of the mix type. "
               "Please change this type to a choice type",
               fileRange);
  }
  ctx->unsetActiveType();
  new IR::MixType(name, opaquedType, {}, mod, subTypesIR, defaultVal, ctx, isPacked, ctx->getVisibInfo(visibSpec),
                  fileRange, None);
}

Json DefineMixType::toJson() const {
  Vec<JsonValue> subTypesJson;
  for (const auto& sub : subtypes) {
    subTypesJson.push_back(Json()
                               ._("name", sub.first)
                               ._("hasType", sub.second.has_value())
                               ._("type", sub.second.has_value() ? sub.second.value()->toJson() : JsonValue()));
  }
  return Json()
      ._("nodeType", "defineMixType")
      ._("name", name)
      ._("subTypes", subTypesJson)
      ._("fileRange", fileRange)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue());
}

} // namespace qat::ast
