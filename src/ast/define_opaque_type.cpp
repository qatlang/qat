#include "./define_opaque_type.hpp"
#include "../IR/types/opaque.hpp"
#include "node.hpp"

namespace qat::ast {

void DefineOpaqueType::define(IR::Context* ctx) {
  if (condition.has_value()) {
    auto cond = condition.value()->emit(ctx);
    if (cond->getType()->isBool()) {
      if (!llvm::cast<llvm::ConstantInt>(cond->getLLVMConstant())->getValue().getBoolValue()) {
        return;
      }
    } else {
      ctx->Error("The condition for defining this opaque type should be of " + ctx->highlightError("bool") +
                     " type. Got an expression with type " + ctx->highlightError(cond->getType()->toString()),
                 condition.value()->fileRange);
    }
  }
  ctx->nameCheckInModule(name, "opaque type", None);
  auto                mod = ctx->getMod();
  Maybe<IR::MetaInfo> irMeta;
  if (metaInfo.has_value()) {
    irMeta = metaInfo.value().toIR(ctx);
  }
  IR::OpaqueType::get(name, {}, None, IR::OpaqueSubtypeKind::core, mod, None, ctx->getVisibInfo(visibSpec), ctx->llctx,
                      irMeta);
}

Json DefineOpaqueType::toJson() const {
  return Json()
      ._("nodeType", "defineOpaqueType")
      ._("name", name)
      ._("visibility", visibSpec.has_value() ? visibSpec.value().toJson() : JsonValue())
      ._("fileRange", fileRange);
}

} // namespace qat::ast