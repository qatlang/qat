#include "./say_sentence.hpp"
#include "../../IR/logic.hpp"
#include "../../cli/config.hpp"

namespace qat::ast {

ir::Value* SayLike::emit(EmitCtx* ctx) {
	auto* cfg = cli::Config::get();
	if ((sayType == SayType::dbg) ? cfg->is_build_mode_debug() : true) {
		SHOW("Say sentence emitting..")
		SHOW("Current block is: " << ctx->get_fn()->get_block()->get_name())
		auto  printfName = ctx->mod->link_internal_dependency(ir::InternalDependency::printf, ctx->irCtx, fileRange);
		auto* printfFn	 = ctx->mod->get_llvm_module()->getFunction(printfName);
		Vec<ir::Value*> valuesIR;
		Vec<FileRange>	valuesRange;
		for (usize i = 0; i < expressions.size(); i++) {
			valuesRange.push_back(expressions[i]->fileRange);
			valuesIR.push_back(expressions[i]->emit(ctx));
		}
		auto fmtRes = ir::Logic::format_values(ctx, valuesIR, {}, fileRange);
		if (sayType != SayType::only) {
			fmtRes.first += "\n";
		}
		Vec<llvm::Value*> values;
		values.push_back(ctx->irCtx->builder.CreateGlobalStringPtr(fmtRes.first, ctx->irCtx->get_global_string_name()));
		for (auto* val : fmtRes.second) {
			values.push_back(val);
		}
		ctx->irCtx->builder.CreateCall(printfFn->getFunctionType(), printfFn, values);
	}
	return nullptr;
}

Json SayLike::to_json() const {
	Vec<JsonValue> exps;
	for (auto* exp : expressions) {
		exps.push_back(exp->to_json());
	}
	return Json()._("nodeType", "saySentence")._("values", exps)._("fileRange", fileRange);
}

} // namespace qat::ast
