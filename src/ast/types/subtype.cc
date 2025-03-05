#include "./subtype.hpp"
#include "../sub_entity_solver.hpp"

namespace qat::ast {

void SubType::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                  EmitCtx* ctx) {
	if (parentType != nullptr) {
		UPDATE_DEPS(parentType);
	}
}

ir::Type* SubType::emit(EmitCtx* ctx) {
	SubEntityResult subRes;
	if (skill.has_value()) {
		if (not ctx->has_skill() && not ctx->has_member_parent()) {
			ctx->Error("Could not find an active skill or skill implementation in the current scope", skill.value());
		}
		if (ctx->has_member_parent() && not ctx->get_member_parent()->is_done_skill()) {
			ctx->Error("Could not find an active skill implementation in the current scope", skill.value());
		}
		subRes = sub_entity_solver(
		    ctx, true,
		    SubEntityParent::of_skill(ctx->has_skill() ? ctx->get_skill()
		                                               : ctx->get_member_parent()->as_done_skill()->get_skill(),
		                              skill.value()),
		    names, fileRange);

	} else if (doneSkill.has_value()) {
		if (not ctx->has_member_parent() || not ctx->get_member_parent()->is_done_skill()) {
			ctx->Error("Could not find an active skill implementation in the current scope", doneSkill.value());
		}
		subRes = sub_entity_solver(
		    ctx, true, SubEntityParent::of_done_skill(ctx->get_member_parent()->as_done_skill(), doneSkill.value()),
		    names, fileRange);
	} else if (parentType != nullptr) {
		auto irTy = parentType->emit(ctx);
		subRes = sub_entity_solver(ctx, true, SubEntityParent::of_type(irTy, parentType->fileRange), names, fileRange);
	}
	if (subRes.isType) {
		return (ir::Type*)subRes.data;
	} else if (((ir::PrerunValue*)subRes.data)->get_ir_type()->is_typed()) {
		return ir::TypeInfo::get_for(((ir::PrerunValue*)subRes.data)->get_llvm_constant())->type;
	} else {
		ctx->Error("Expected a type here, but instead got an expression of type " +
		               ctx->color(((ir::PrerunValue*)subRes.data)->get_ir_type()->to_string()),
		           fileRange);
	}
}

} // namespace qat::ast
