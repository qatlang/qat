#include "./type_definition.hpp"
#include "./types/generic_abstract.hpp"
#include "expression.hpp"

namespace qat::ast {

TypeDefinition::TypeDefinition(Identifier _name, Maybe<PrerunExpression*> _checker,
                               Vec<ast::GenericAbstractType*> _generics, Maybe<PrerunExpression*> _constraint,
                               QatType* _subType, FileRange _fileRange, Maybe<VisibilitySpec> _visibSpec)
    : Node(std::move(_fileRange)), name(std::move(_name)), checker(_checker), subType(_subType),
      constraint(_constraint), visibSpec(_visibSpec), generics(_generics) {}

bool TypeDefinition::isGeneric() const { return !generics.empty(); }

void TypeDefinition::setVariantName(const String& name) const { variantName = name; }

void TypeDefinition::unsetVariantName() const { variantName = None; }

void TypeDefinition::createType(IR::Context* ctx) const {
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
  auto* mod = ctx->getMod();
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
  typeDefinition = new IR::DefinitionType(dTyName, subType->emit(ctx), genericsIR, mod, ctx->getVisibInfo(visibSpec));
}

void TypeDefinition::defineType(IR::Context* ctx) {
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  if (isGeneric()) {
    genericTypeDefinition =
        new IR::GenericDefinitionType(name, generics, constraint, this, ctx->getMod(), ctx->getVisibInfo(visibSpec));
  } else {
    createType(ctx);
  }
}

void TypeDefinition::define(IR::Context* ctx) {
  if (checkResult.has_value() && !checkResult.value()) {
    return;
  }
  if (isGeneric()) {
    createType(ctx);
  }
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