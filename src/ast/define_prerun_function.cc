#include "./define_prerun_function.hpp"
#include "../IR/prerun_function.hpp"
#include "../IR/types/void.hpp"
#include "./expression.hpp"
#include "./prerun_sentences/prerun_sentence.hpp"

namespace qat::ast {

void DefinePrerunFunction::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	mod->entity_name_check(irCtx, name, ir::EntityType::prerunFunction);
	entityState = mod->add_entity(name, ir::EntityType::prerunFunction, this, ir::EmitPhase::phase_2);
}

void DefinePrerunFunction::update_entity_dependencies(ir::Mod* mod, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, mod);
	if (defineChecker != nullptr) {
		defineChecker->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
	if (returnType != nullptr) {
		returnType->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
	}
	for (auto* arg : arguments) {
		if (arg->get_type()) {
			arg->get_type()->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
		}
	}
	for (auto* snt : sentences) {
		snt->update_dependencies(ir::EmitPhase::phase_2, ir::DependType::complete, entityState, ctx);
	}
}

void DefinePrerunFunction::do_phase(ir::EmitPhase phase, ir::Mod* mod, ir::Ctx* irCtx) {
	if ((phase == ir::EmitPhase::phase_1) && (defineChecker != nullptr)) {
		auto ctx      = EmitCtx::get(irCtx, mod);
		auto checkRes = defineChecker->emit(ctx);
		if (not checkRes->get_ir_type()->is_bool()) {
			ctx->Error("Define condition is required to be of type " + ctx->color("bool") +
			               ", but found an expression of type " + ctx->color(checkRes->get_ir_type()->to_string()),
			           defineChecker->fileRange);
		}
		if (not llvm::cast<llvm::ConstantInt>(checkRes->get_llvm_constant())->getValue().getBoolValue()) {
			entityState->complete_manually();
			return;
		}
	} else if (phase == ir::EmitPhase::phase_2) {
		create_function(EmitCtx::get(irCtx, mod));
	}
}

void DefinePrerunFunction::create_function(EmitCtx* ctx) {
	auto                   irReturnTy = returnType ? returnType->emit(ctx) : ir::VoidType::get(ctx->irCtx->llctx);
	Vec<ir::ArgumentType*> irArgs;
	for (usize i = 0; i < arguments.size(); i++) {
		auto arg = arguments[i];
		if (arg->is_variadic_arg() && (i != (arguments.size() - 1))) {
			ctx->Error("Variadic argument should be the last argument", arg->get_name().range);
		} else if (arg->is_member_arg()) {
			ctx->Error("Member arguments are not allowed for normal functions", arg->get_name().range);
		}
		auto argTy = arg->get_type()->emit(ctx);
		irArgs.push_back(ir::ArgumentType::create_normal(argTy, arg->get_name().value, arg->is_variable()));
	}
	// TODO - Analyze control flow of sentences
	(void)ir::PrerunFunction::create(ctx->mod, name, irReturnTy, irArgs, std::make_pair(sentences, fileRange),
	                                 ctx->get_visibility_info(visibSpec), ctx->irCtx->llctx);
}

} // namespace qat::ast
