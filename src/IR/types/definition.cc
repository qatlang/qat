#include "./definition.hpp"
#include "../../ast/expression.hpp"
#include "../../ast/type_definition.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

DefinitionType::DefinitionType(Identifier _name, Type* _subType, Vec<GenericArgument*> _generics, Mod* _mod,
                               const VisibilityInfo& _visibInfo)
    : ExpandedType(_name, _generics, _mod, _visibInfo), EntityOverview("typeDefinition", Json(), _name.range),
      subType(_subType) {
  setSubType(subType);
  linkingName = subType->get_name_for_linking();
  parent->typeDefs.push_back(this);
}

LinkNames DefinitionType::get_link_names() const {
  return LinkNames({LinkNameUnit(linkingName, LinkUnitType::typeName)}, None, nullptr);
}

void DefinitionType::setSubType(Type* _subType) {
  subType = _subType;
  if (subType) {
    llvmType = subType->get_llvm_type();
  }
}

VisibilityInfo DefinitionType::get_visibility() const { return visibility; }

bool DefinitionType::is_expanded() const { return subType->is_expanded(); }

bool DefinitionType::is_copy_constructible() const { return subType->is_copy_constructible(); }

bool DefinitionType::is_copy_assignable() const { return subType->is_copy_assignable(); }

bool DefinitionType::is_move_constructible() const { return subType->is_move_constructible(); }

bool DefinitionType::is_move_assignable() const { return subType->is_move_assignable(); }

bool DefinitionType::is_destructible() const { return subType->is_destructible(); }

void DefinitionType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  subType->copy_construct_value(irCtx, first, second, fun);
}

void DefinitionType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  subType->copy_assign_value(irCtx, first, second, fun);
}

void DefinitionType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  subType->move_construct_value(irCtx, first, second, fun);
}

void DefinitionType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
  subType->move_assign_value(irCtx, first, second, fun);
}

void DefinitionType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
  subType->destroy_value(irCtx, instance, fun);
}

Identifier DefinitionType::get_name() const { return name; }

String DefinitionType::get_full_name() const {
  return parent ? parent->get_fullname_with_child(name.value) : name.value;
}

Mod* DefinitionType::get_module() { return parent; }

Type* DefinitionType::get_subtype() { return subType; }

bool DefinitionType::is_type_sized() const { return subType->is_type_sized(); }

bool DefinitionType::is_trivially_copyable() const { return subType->is_trivially_copyable(); }

bool DefinitionType::is_trivially_movable() const { return subType->is_trivially_movable(); }

void DefinitionType::update_overview() {
  Vec<JsonValue> genJson;
  for (auto* gen : generics) {
    genJson.push_back(gen->to_json());
  }
  ovInfo._("fullName", get_full_name())
      ._("typeID", get_id())
      ._("subTypeID", subType->get_id())
      ._("visibility", visibility)
      ._("hasGenerics", !generics.empty())
      ._("generics", genJson)
      ._("moduleID", parent->get_id());
}

Maybe<String> DefinitionType::to_prerun_generic_string(ir::PrerunValue* constant) const {
  if (subType->can_be_prerun_generic()) {
    return subType->to_prerun_generic_string(constant);
  } else {
    return None;
  }
}

Maybe<bool> DefinitionType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
  if (subType->can_be_prerun_generic()) {
    return subType->equality_of(irCtx, first, second);
  } else {
    return None;
  }
}

TypeKind DefinitionType::type_kind() const { return TypeKind::definition; }

String DefinitionType::to_string() const { return get_full_name(); }

GenericDefinitionType::GenericDefinitionType(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                             Maybe<ast::PrerunExpression*> _constraint,
                                             ast::TypeDefinition* _defineTypeDef, Mod* _parent,
                                             const VisibilityInfo& _visibInfo)
    : EntityOverview("genericTypeDefinition",
                     Json()
                         ._("name", _name.value)
                         ._("fullName", _parent->get_fullname_with_child(_name.value))
                         ._("visibility", _visibInfo)
                         ._("moduleID", _parent->get_id()),
                     _name.range),
      name(_name), generics(_generics), defineTypeDef(_defineTypeDef), parent(_parent), visibility(_visibInfo),
      constraint(_constraint) {
  parent->genericTypeDefinitions.push_back(this);
}

Identifier GenericDefinitionType::get_name() const { return name; }

VisibilityInfo GenericDefinitionType::get_visibility() const { return visibility; }

bool GenericDefinitionType::all_generics_have_defaults() const {
  for (auto* gen : generics) {
    if (!gen->hasDefault()) {
      return false;
    }
  }
  return true;
}

usize GenericDefinitionType::get_generic_count() const { return generics.size(); }

usize GenericDefinitionType::get_variant_count() const { return variants.size(); }

Mod* GenericDefinitionType::get_module() const { return parent; }

ast::GenericAbstractType* GenericDefinitionType::get_generic_at(usize index) const { return generics.at(index); }

DefinitionType* GenericDefinitionType::fill_generics(Vec<GenericToFill*>& types, ir::Ctx* irCtx, FileRange range) {
  for (auto var : variants) {
    if (var.check(irCtx, [&](const String& msg, const FileRange& rng) { irCtx->Error(msg, rng); }, types)) {
      return var.get();
    }
  }
  ir::fill_generics(irCtx, generics, types, range);
  if (constraint.has_value()) {
    auto checkVal = constraint.value()->emit(ast::EmitCtx::get(irCtx, parent));
    if (checkVal->get_ir_type()->is_bool()) {
      if (!llvm::cast<llvm::ConstantInt>(checkVal->get_llvm_constant())->getValue().getBoolValue()) {
        irCtx->Error("The provided generic parameters for the generic function do not satisfy the constraints", range,
                     Pair<String, FileRange>{"The constraint can be found here", constraint.value()->fileRange});
      }
    } else {
      irCtx->Error("The constraints for generic parameters should be of " + irCtx->color("bool") +
                       " type. Got an expression of " + irCtx->color(checkVal->get_ir_type()->to_string()),
                   constraint.value()->fileRange);
    }
  }
  Vec<ir::GenericArgument*> genParams;
  for (auto genAb : generics) {
    genParams.push_back(genAb->toIRGenericType());
  }
  auto variantName = ir::Logic::get_generic_variant_name(name.value, types);
  defineTypeDef->set_variant_name(variantName);
  irCtx->add_active_generic(
      ir::GenericEntityMarker{
          variantName,
          ir::GenericEntityType::typeDefinition,
          range,
          0u,
          genParams,
      },
      true);
  (void)defineTypeDef->create_type(parent, irCtx);
  auto* dTy = defineTypeDef->getDefinition();
  variants.push_back(GenericVariant<DefinitionType>(dTy, types));
  for (auto* temp : generics) {
    temp->unset();
  }
  defineTypeDef->unset_variant_name();
  if (irCtx->get_active_generic().warningCount > 0) {
    auto count = irCtx->get_active_generic().warningCount;
    irCtx->remove_active_generic();
    irCtx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
                       " generated while creating generic variant " + irCtx->highlightWarning(variantName),
                   range);
  } else {
    irCtx->remove_active_generic();
  }
  return dTy;
}

} // namespace qat::ir
