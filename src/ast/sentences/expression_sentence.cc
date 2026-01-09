#include "./expression_sentence.hpp"

namespace qat::ast {

ir::Value* ExpressionSentence::emit(EmitCtx* ctx) { return expr->emit(ctx); }

} // namespace qat::ast
