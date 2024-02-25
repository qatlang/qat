#include "./type_definition.hpp"
#include "./types/generic_abstract.hpp"
#include "./types/prerun_generic.hpp"
#include "./types/typed_generic.hpp"
#include "expression.hpp"

namespace qat::ast {

void TypeDefinition::create_entity(IR::QatModule* mod, IR::Context* ctx) {
  SHOW("CreateEntity: " << name.value)
  typeSize = subType->getTypeSizeInBits(ctx);
  mod->entity_name_check(ctx, name, isGeneric() ? IR::EntityType::genericTypeDef : IR::EntityType::typeDefinition);
  entityState = mod->add_entity(name, isGeneric() ? IR::EntityType::genericTypeDef : IR::EntityType::typeDefinition,
                                this, IR::EmitPhase::phase_1);
}

void TypeDefinition::update_entity_dependencies(IR::QatModule* mod, IR::Context* ctx) {
  if (checker.has_value()) {
    checker.value()->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
  }
  if (isGeneric()) {
    for (auto gen : generics) {
      if (gen->isPrerun() && gen->asPrerun()->hasDefault()) {
        gen->asPrerun()->getDefaultAST()->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete,
                                                              entityState, ctx);
      } else if (gen->isTyped() && gen->asTyped()->hasDefault()) {
        gen->asTyped()->getDefaultAST()->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete,
                                                             entityState, ctx);
      }
    }
  }
  subType->update_dependencies(IR::EmitPhase::phase_1, IR::DependType::complete, entityState, ctx);
}

void TypeDefinition::do_phase(IR::EmitPhase phase, IR::QatModule* mod, IR::Context* ctx) {
  if (checker.has_value()) {
    auto* checkRes = checker.value()->emit(ctx);
    if (checkRes->getType()->isBool()) {
      checkResult = llvm::cast<llvm::ConstantInt>(checkRes->getLLVMConstant())->getValue().getBoolValue();
      if (!checkResult.value()) {
        // TODO - ADD THIS AS DEAD CODE IN CODE INFO
        return;
      }
    } else {
      ctx->Error("The condition for this type definition should be of " + ctx->highlightError("bool") +
                     " type. Got an expression of type " + ctx->highlightError(checkRes->getType()->toString()),
                 checker.value()->fileRange);
    }
  }
  if (isGeneric()) {
    genericTypeDefinition =
        new IR::GenericDefinitionType(name, generics, constraint, this, ctx->getMod(), ctx->getVisibInfo(visibSpec));
  } else {
    create_type(mod, ctx);
  }
}

bool TypeDefinition::isGeneric() const { return !generics.empty(); }

void TypeDefinition::setVariantName(const String& name) const { variantName = name; }

void TypeDefinition::unsetVariantName() const { variantName = None; }

void TypeDefinition::create_type(IR::QatModule* mod, IR::Context* ctx) const {
  ctx->nameCheckInModule(name, isGeneric() ? "generic type definition" : "type definition",
                         isGeneric() ? Maybe<String>(genericTypeDefinition->getID()) : None);
  auto dTyName = name;
  if (isGeneric()) {
    dTyName = Identifier(variantName.value(), name.range);
  }
  Vec<IR::GenericParameter*> genericsIR;
  for (auto* gen : generics) {
    if (!gen->isSet()) {
      if (gen->isTyped()) {
        ctx->Error("No type is set for the generic type " + ctx->highlightError(gen->getName().value) +
                       " and there is no default type provided",
                   gen->getRange());
      } else if (gen->isPrerun()) {
        ctx->Error("No value is set for the generic prerun expression " + ctx->highlightError(gen->getName().value) +
                       " and there is no default expression provided",
                   gen->getRange());
      } else {
        ctx->Error("Invalid generic kind", gen->getRange());
      }
    }
    genericsIR.push_back(gen->toIRGenericType());
  }
  if (isGeneric()) {
    ctx->getActiveGeneric().generics = genericsIR;
  }
  SHOW("Type definition " << dTyName.value)
  typeDefinition = new IR::DefinitionType(dTyName, subType->emit(ctx), genericsIR, mod, ctx->getVisibInfo(visibSpec));
}

IR::DefinitionType* TypeDefinition::getDefinition() const { return typeDefinition; }

Json TypeDefinition::toJson() const {
  Vec<JsonValue> genJson;
  for (auto* gen : generics) {
    genJson.push_back(gen->toJson());
  }
  return Json()
      ._("nodeType", "typeDefinition")
      ._("name", name)
      ._("subType", subType->toJson())
      ._("hasGenerics", !generics.empty())
      ._("generics", genJson)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast