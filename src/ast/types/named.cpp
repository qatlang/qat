#include "./named.hpp"
#include "../../IR/types/region.hpp"
#include "../../utils/split_string.hpp"

namespace qat::ast {

NamedType::NamedType(u32 _relative, String _name, const bool _variable, utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), relative(_relative), name(std::move(_name)) {}

IR::QatType* NamedType::emit(IR::Context* ctx) {
  auto* mod     = ctx->getMod();
  auto  reqInfo = ctx->getReqInfo();
  if (relative != 0) {
    if (ctx->getMod()->hasNthParent(relative)) {
      mod = ctx->getMod()->getNthParent(relative);
    } else {
      ctx->Error("The active scope does not have " + std::to_string(relative) + " parents", fileRange);
    }
  }
  auto entityName = name;
  if (name.find(':') != String::npos) {
    auto splits = utils::splitString(name, ":");
    entityName  = splits.back();
    for (usize i = 0; i < (splits.size() - 1); i++) {
      auto split = splits.at(i);
      if (mod->hasLib(split)) {
        mod = mod->getLib(split, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Lib " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
        }
      } else if (mod->hasBox(split)) {
        mod = mod->getBox(split, reqInfo);
        if (!mod->getVisibility().isAccessible(reqInfo)) {
          ctx->Error("Box " + ctx->highlightError(mod->getFullName()) + " is not accessible here", fileRange);
        }
      } else {
        ctx->Error("No box or lib named " + ctx->highlightError(split) + " found inside " +
                       ctx->highlightError(mod->getFullName()),
                   fileRange);
      }
    }
  }
  if (mod->hasCoreType(entityName) || mod->hasBroughtCoreType(entityName) ||
      mod->hasAccessibleCoreTypeInImports(entityName, reqInfo).first) {
    auto* cTy = mod->getCoreType(entityName, reqInfo);
    if (!cTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Core type " + ctx->highlightError(cTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 fileRange);
    }
    return cTy;
  } else if (mod->hasTypeDef(entityName) || mod->hasBroughtTypeDef(entityName) ||
             mod->hasAccessibleTypeDefInImports(entityName, reqInfo).first) {
    auto* dTy = mod->getTypeDef(entityName, reqInfo);
    if (!dTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Type definition " + ctx->highlightError(dTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 fileRange);
    }
    return dTy;
  } else if (mod->hasMixType(entityName) || mod->hasBroughtMixType(entityName) ||
             mod->hasAccessibleMixTypeInImports(entityName, reqInfo).first) {
    auto* mTy = mod->getMixType(entityName, reqInfo);
    if (!mTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Mix type " + ctx->highlightError(mTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 fileRange);
    }
    return mTy;
  } else if (mod->hasChoiceType(entityName) || mod->hasBroughtChoiceType(entityName) ||
             mod->hasAccessibleChoiceTypeInImports(entityName, reqInfo).first) {
    auto* chTy = mod->getChoiceType(entityName, reqInfo);
    if (!chTy->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Choice type " + ctx->highlightError(chTy->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 fileRange);
    }
    return chTy;
  } else if (mod->hasRegion(entityName) || mod->hasBroughtRegion(entityName) ||
             mod->hasAccessibleRegionInImports(entityName, reqInfo).first) {
    auto* reg = mod->getRegion(entityName, reqInfo);
    if (!reg->getVisibility().isAccessible(reqInfo)) {
      ctx->Error("Region " + ctx->highlightError(reg->getFullName()) + " inside module " +
                     ctx->highlightError(mod->getFullName()) + " is not accessible here",
                 fileRange);
    }
    return reg;
  } else {
    ctx->Error("No type named " + name + " found in scope", fileRange);
  }
  return nullptr;
}

String NamedType::getName() const { return name; }

u32 NamedType::getRelative() const { return relative; }

TypeKind NamedType::typeKind() const { return TypeKind::Float; }

Json NamedType::toJson() const {
  return Json()._("typeKind", "named")._("name", name)._("isVariable", isVariable())._("fileRange", fileRange);
}

String NamedType::toString() const { return (isVariable() ? "var " : "") + name; }

} // namespace qat::ast