#include "./char.hpp"
#include "../../IR/types/char.hpp"

namespace qat::ast {

ir::Type* CharType::emit(EmitCtx* ctx) { return ir::CharType::get(ctx->irCtx->llctx); }

} // namespace qat::ast
