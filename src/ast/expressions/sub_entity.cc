#include "./sub_entity.hpp"
#include "../../IR/types/typed.hpp"
#include "../sub_entity_solver.hpp"

namespace qat::ast {

void SubEntity::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                    EmitCtx* ctx) {
	if (parentType) {
		parentType.update_dependencies(phase, dep, ent, ctx);
	}
}

ir::Value* SubEntity::emit(EmitCtx* ctx) {
	SubEntityResult subRes;
	if (skill.has_value()) {
		if (not ctx->has_skill() && (not ctx->has_member_parent() || not ctx->get_member_parent()->is_done_skill())) {
			ctx->Error("Could not find an active skill or skill implementation in the current scope", skill.value());
		}
		subRes = sub_entity_solver(
		    ctx, false,
		    SubEntityParent::of_skill(ctx->has_skill() ? ctx->get_skill()
		                                               : ctx->get_member_parent()->as_done_skill()->get_skill(),
		                              skill.value()),
		    names, fileRange);
	} else if (doneSkill.has_value()) {
		if (not ctx->has_member_parent() || not ctx->get_member_parent()->is_done_skill()) {
			ctx->Error("Could not find an active skill implementation in the current scope", doneSkill.value());
		}
		subRes = sub_entity_solver(
		    ctx, false, SubEntityParent::of_done_skill(ctx->get_member_parent()->as_done_skill(), doneSkill.value()),
		    names, fileRange);
	} else if (parentType) {
		auto irTy = parentType.emit(ctx);
		subRes =
		    sub_entity_solver(ctx, false, SubEntityParent::of_type(irTy, parentType.get_range()), names, fileRange);
	}
	if (subRes.isType) {
		return ir::PrerunValue::get(ir::TypeInfo::create(ctx->irCtx, (ir::Type*)subRes.data, ctx->mod)->id,
		                            ir::TypedType::get(ctx->irCtx));
	} else {
		return (ir::Value*)subRes.data;
	}
}

} // namespace qat::ast
