#include "./expression_sentence.hpp"

namespace qat::ast {

ir::Value* ExpressionSentence::emit(EmitCtx* ctx) { return expr->emit(ctx); }

Json ExpressionSentence::to_json() const {
	return Json()._("nodeType", "expressionSentence")._("value", expr->to_json())._("fileRange", fileRange);
}

} // namespace qat::ast