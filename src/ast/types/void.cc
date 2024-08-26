#include "../../IR/types/void.hpp"
#include "./void.hpp"

namespace qat::ast {

ir::Type* VoidType::emit(EmitCtx* ctx) { return ir::VoidType::get(ctx->irCtx->llctx); }

AstTypeKind VoidType::type_kind() const { return AstTypeKind::VOID; }

Json VoidType::to_json() const { return Json()._("typeKind", "void")._("fileRange", fileRange); }

String VoidType::to_string() const { return "void"; }

} // namespace qat::ast