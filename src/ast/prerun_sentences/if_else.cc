#include "./if_else.hpp"
#include "../expression.hpp"

namespace qat::ast {

void PrerunIfElse::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                       EmitCtx* ctx) {
	UPDATE_DEPS(ifBlock.first);
	for (auto* snt : ifBlock.second) {
		UPDATE_DEPS(snt);
	}
	for (auto ch : elseIfChain) {
		UPDATE_DEPS(ch.first);
		for (auto* snt : ch.second) {
			UPDATE_DEPS(snt);
		}
	}
	if (elseBlock.has_value()) {
		for (auto* snt : elseBlock.value()) {
			UPDATE_DEPS(snt);
		}
	}
}

void PrerunIfElse::emit(EmitCtx* ctx) {
	auto ifCond = ifBlock.first->emit(ctx);
	if (not ifCond->get_ir_type()->is_bool()) {
		ctx->Error("This condition is expected to be of type " + ctx->color("bool") +
		               " but found an expression of type " + ctx->color(ifCond->get_ir_type()->to_string()),
		           ifBlock.first->fileRange);
	}
	if (llvm::cast<llvm::ConstantInt>(ifCond->get_llvm_constant())->getValue().getBoolValue()) {
		auto block = ir::PreBlock::create_next_to(ctx->get_pre_call_state()->get_block());
		block->make_active();
		for (auto* snt : ifBlock.second) {
			snt->emit(ctx);
		}
		ir::PreBlock::create_next_to(block)->make_active();
	} else {
		// IF CONDITION IS FALSE
		bool executedBlock = false;
		for (auto& ch : elseIfChain) {
			auto cond = ch.first->emit(ctx);
			if (not cond->get_ir_type()->is_bool()) {
				ctx->Error("This condition is expected to be of type " + ctx->color("bool") +
				               " but found an expression of type " + ctx->color(cond->get_ir_type()->to_string()),
				           ch.first->fileRange);
			}
			if (llvm::cast<llvm::ConstantInt>(cond->get_llvm_constant())->getValue().getBoolValue()) {
				executedBlock = true;
				auto block    = ir::PreBlock::create_next_to(ctx->get_pre_call_state()->get_block());
				block->make_active();
				for (auto* snt : ch.second) {
					snt->emit(ctx);
				}
				ir::PreBlock::create_next_to(block)->make_active();
				break;
			}
		}
		if (not executedBlock && elseBlock.has_value()) {
			auto block = ir::PreBlock::create_next_to(ctx->get_pre_call_state()->get_block());
			block->make_active();
			for (auto* snt : elseBlock.value()) {
				snt->emit(ctx);
			}
			ir::PreBlock::create_next_to(block)->make_active();
		}
	}
}

} // namespace qat::ast
