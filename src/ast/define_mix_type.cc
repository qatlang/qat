#include "./define_mix_type.hpp"
#include "./expression.hpp"
#include <cmath>

namespace qat::ast {

void DefineMixType::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
  SHOW("CreateEntity: " << name.value)
  mod->entity_name_check(irCtx, name, ir::EntityType::mixType);
  entityState = mod->add_entity(name, ir::EntityType::mixType, this, ir::EmitPhase::phase_2);
}

void DefineMixType::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
  auto emitCtx = EmitCtx::get(irCtx, mod);
  for (auto& sub : subtypes) {
    if (sub.second.has_value()) {
      sub.second.value()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, emitCtx);
    }
  }
}

void DefineMixType::create_opaque(ir::Mod* mod, ir::Ctx* irCtx) {
  usize maxSubtypeSize          = 8u;
  bool  foundSizeForAssociation = true;
  usize tagBitwidth             = 1u;
  while (std::pow(2, tagBitwidth) < (subtypes.size() + 1)) {
    tagBitwidth++;
  }
  for (auto& subty : subtypes) {
    if (subty.second.has_value()) {
      auto subTySize = subty.second.value()->getTypeSizeInBits(EmitCtx::get(irCtx, mod));
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
  opaquedType = ir::OpaqueType::get(
      name, {}, None, ir::OpaqueSubtypeKind::mix, mod,
      foundSizeForAssociation
          ? Maybe<usize>(irCtx->dataLayout->getTypeAllocSizeInBits(llvm::StructType::get(
                irCtx->llctx,
                {llvm::Type::getIntNTy(irCtx->llctx, tagBitwidth), llvm::Type::getIntNTy(irCtx->llctx, maxSubtypeSize)},
                isPacked)))
          : None,
      EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), irCtx->llctx, None);
}

void DefineMixType::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
  auto ctx = EmitCtx::get(irCtx, mod);
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  } else if (defineChecker) {
    auto checkRes = defineChecker->emit(ctx);
    if (checkRes->get_ir_type()->is_bool()) {
      checkResult = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
      if (!checkResult.value()) {
        return;
      }
    } else {
      ctx->Error("The define condition is expected to be of " + ctx->color("bool") + " type", defineChecker->fileRange);
    }
  }
  if (phase == ir::EmitPhase::phase_1) {
    create_opaque(mod, irCtx);
  } else if (phase == ir::EmitPhase::phase_2) {
    create_type(mod, irCtx);
  }
}

void DefineMixType::create_type(ir::Mod* mod, ir::Ctx* irCtx) {
  Vec<Pair<Identifier, Maybe<ir::Type*>>> subTypesIR;
  bool                                    hasAssociatedType = false;
  for (usize i = 0; i < subtypes.size(); i++) {
    for (usize j = i + 1; j < subtypes.size(); j++) {
      if (subtypes.at(i).first.value == subtypes.at(j).first.value) {
        irCtx->Error("The name of the subtype of the mix is repeating here. Please "
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
          irCtx->Error("A subfield with a value associated with them cannot be "
                       "used as a default value",
                       FileRange{fRanges.at(i), subtypes.at(i).second.value()->fileRange});
        }
      }
    }
    auto emitCtx = EmitCtx::get(irCtx, mod);
    if (opaquedType) {
      emitCtx->with_opaque_parent(opaquedType);
    }
    auto subTypeTy = subtypes.at(i).second.has_value() ? subtypes.at(i).second.value()->emit(emitCtx) : nullptr;
    if (subtypes.at(i).second.has_value() && subTypeTy->is_opaque() && !subTypeTy->as_opaque()->is_type_sized()) {
      if (opaquedType && subTypeTy->as_opaque()->is_same(opaquedType)) {
        irCtx->Error(
            "Type nesting found. The variant " + irCtx->color(subtypes.at(i).first.value) + " of mix type " +
                irCtx->color(opaquedType->to_string()) +
                " has the same type associated with it as its parent. Check the code for mistakes or use a mark or reference to the parent type as the variant type",
            subtypes.at(i).second.value()->fileRange);
      } else {
        irCtx->Error("The variant " + irCtx->color(subtypes.at(i).first.value) + " of mix type " +
                         irCtx->color(opaquedType->to_string()) +
                         " has an incomplete type with an unknown size associated with it",
                     subtypes.at(i).second.value()->fileRange);
      }
    }
    subTypesIR.push_back(Pair<Identifier, Maybe<ir::Type*>>(
        subtypes.at(i).first, subtypes.at(i).second.has_value() ? Maybe<ir::Type*>(subTypeTy) : None));
  }
  if (!hasAssociatedType) {
    irCtx->Error("No types associated to any of the subfields of the mix type. "
                 "Please change this type to a choice type",
                 fileRange);
  }
  new ir::MixType(name, opaquedType, {}, mod, subTypesIR, defaultVal, irCtx, isPacked,
                  EmitCtx::get(irCtx, mod)->get_visibility_info(visibSpec), fileRange, None);
}

Json DefineMixType::to_json() const {
  Vec<JsonValue> subTypesJson;
  for (const auto& sub : subtypes) {
    subTypesJson.push_back(Json()
                               ._("name", sub.first)
                               ._("hasType", sub.second.has_value())
                               ._("type", sub.second.has_value() ? sub.second.value()->to_json() : JsonValue()));
  }
  return Json()
      ._("nodeType", "defineMixType")
      ._("name", name)
      ._("subTypes", subTypesJson)
      ._("fileRange", fileRange)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue());
}

} // namespace qat::ast
