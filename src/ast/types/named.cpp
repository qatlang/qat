#include "./named.hpp"

namespace qat::ast {

NamedType::NamedType(u32 _relative, String _name, const bool _variable,
                     utils::FileRange _fileRange)
    : QatType(_variable, std::move(_fileRange)), relative(_relative),
      name(std::move(_name)) {}

IR::QatType *NamedType::emit(IR::Context *ctx) {
  // FIXME - Implement remaining logic
  // FIXME - Support sum types
  if (relative != 0) {
    if (ctx->getMod()->hasNthParent(relative)) {
      auto *mod = ctx->getMod()->getNthParent(relative);
      if (mod->hasCoreType(name) || mod->hasBroughtCoreType(name) ||
          mod->hasAccessibleCoreTypeInImports(name, ctx->getReqInfo()).first) {
        return mod->getCoreType(name, ctx->getReqInfo());
      } else if (mod->hasTypeDef(name) || mod->hasBroughtTypeDef(name) ||
                 mod->hasAccessibleTypeDefInImports(name, ctx->getReqInfo())
                     .first) {
        return mod->getTypeDef(name, ctx->getReqInfo());
      }
    } else {
      ctx->Error("The active scope does not have " + std::to_string(relative) +
                     " parents",
                 fileRange);
    }
  } else {
    auto *mod = ctx->getMod();
    if (mod->hasCoreType(name) || mod->hasBroughtCoreType(name) ||
        mod->hasAccessibleCoreTypeInImports(name, ctx->getReqInfo()).first) {
      return mod->getCoreType(name, ctx->getReqInfo());
    } else if (mod->hasTypeDef(name) || mod->hasBroughtTypeDef(name) ||
               mod->hasAccessibleTypeDefInImports(name, ctx->getReqInfo())
                   .first) {
      return mod->getTypeDef(name, ctx->getReqInfo());
    } else {
      ctx->Error("No type named " + name + " found in scope", fileRange);
    }
  }
  return nullptr;
}

String NamedType::getName() const { return name; }

u32 NamedType::getRelative() const { return relative; }

TypeKind NamedType::typeKind() const { return TypeKind::Float; }

nuo::Json NamedType::toJson() const {
  return nuo::Json()
      ._("typeKind", "named")
      ._("name", name)
      ._("isVariable", isVariable())
      ._("fileRange", fileRange);
}

String NamedType::toString() const {
  return (isVariable() ? "var " : "") + name;
}

} // namespace qat::ast