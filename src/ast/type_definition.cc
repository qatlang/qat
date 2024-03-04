#include "./type_definition.hpp"
#include "./types/generic_abstract.hpp"
#include "./types/prerun_generic.hpp"
#include "./types/typed_generic.hpp"
#include "expression.hpp"

namespace qat::ast {

void TypeDefinition::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
  SHOW("CreateEntity: " << name.value)
  typeSize = subType->getTypeSizeInBits(EmitCtx::get(irCtx, mod));
  mod->entity_name_check(irCtx, name, isGeneric() ? ir::EntityType::genericTypeDef : ir::EntityType::typeDefinition);
  entityState = mod->add_entity(name, isGeneric() ? ir::EntityType::genericTypeDef : ir::EntityType::typeDefinition,
                                this, ir::EmitPhase::phase_1);
}

void TypeDefinition::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
  auto ctx = EmitCtx::get(irCtx, mod);
  if (checker.has_value()) {
    checker.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
  }
  if (isGeneric()) {
    for (auto gen : generics) {
      if (gen->is_prerun() && gen->as_prerun()->hasDefault()) {
        gen->as_prerun()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete,
                                                               entityState, ctx);
      } else if (gen->is_typed() && gen->as_typed()->hasDefault()) {
        gen->as_typed()->getDefaultAST()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete,
                                                              entityState, ctx);
      }
    }
  }
  subType->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
}

void TypeDefinition::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
  auto emitCtx = EmitCtx::get(irCtx, mod);
  if (checker.has_value()) {
    auto* checkRes = checker.value()->emit(emitCtx);
    if (checkRes->get_ir_type()->is_bool()) {
      checkResult = llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue();
      if (!checkResult.value()) {
        // TODO - ADD THIS AS DEAD CODE IN CODE INFO
        return;
      }
    } else {
      irCtx->Error("The condition for this type definition should be of " + irCtx->color("bool") +
                       " type. Got an expression of type " + irCtx->color(checkRes->get_ir_type()->to_string()),
                   checker.value()->fileRange);
    }
  }
  if (isGeneric()) {
    genericTypeDefinition =
        new ir::GenericDefinitionType(name, generics, constraint, this, mod, emitCtx->getVisibInfo(visibSpec));
  } else {
    create_type(mod, irCtx);
  }
}

bool TypeDefinition::isGeneric() const { return !generics.empty(); }

void TypeDefinition::setVariantName(const String& name) const { variantName = name; }

void TypeDefinition::unsetVariantName() const { variantName = None; }

void TypeDefinition::create_type(ir::Mod* mod, ir::Ctx* irCtx) const {
  auto emitCtx = EmitCtx::get(irCtx, mod);
  emitCtx->name_check_in_module(name, isGeneric() ? "generic type definition" : "type definition",
                                isGeneric() ? Maybe<String>(genericTypeDefinition->get_id()) : None);
  auto dTyName = name;
  if (isGeneric()) {
    dTyName = Identifier(variantName.value(), name.range);
  }
  Vec<ir::GenericParameter*> genericsIR;
  for (auto* gen : generics) {
    if (!gen->isSet()) {
      if (gen->is_typed()) {
        irCtx->Error("No type is set for the generic type " + irCtx->color(gen->get_name().value) +
                         " and there is no default type provided",
                     gen->get_range());
      } else if (gen->as_typed()) {
        irCtx->Error("No value is set for the generic prerun expression " + irCtx->color(gen->get_name().value) +
                         " and there is no default expression provided",
                     gen->get_range());
      } else {
        irCtx->Error("Invalid generic kind", gen->get_range());
      }
    }
    genericsIR.push_back(gen->toIRGenericType());
  }
  if (isGeneric()) {
    irCtx->getActiveGeneric().generics = genericsIR;
  }
  SHOW("Type definition " << dTyName.value)
  typeDefinition =
      new ir::DefinitionType(dTyName, subType->emit(emitCtx), genericsIR, mod, emitCtx->getVisibInfo(visibSpec));
}

ir::DefinitionType* TypeDefinition::getDefinition() const { return typeDefinition; }

Json TypeDefinition::to_json() const {
  Vec<JsonValue> genJson;
  for (auto* gen : generics) {
    genJson.push_back(gen->to_json());
  }
  return Json()
      ._("nodeType", "typeDefinition")
      ._("name", name)
      ._("subType", subType->to_json())
      ._("hasGenerics", !generics.empty())
      ._("generics", genJson)
      ._("hasVisibility", visibSpec.has_value())
      ._("visibility", visibSpec.has_value() ? visibSpec->to_json() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast