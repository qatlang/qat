#include "./sub_entity.hpp"
#include "../sub_entity_solver.hpp"
#include "../types/qat_type.hpp"

namespace qat::ast {

void PrerunSubEntity::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
                                          EmitCtx* ctx) {
	if (parentType != nullptr) {
		UPDATE_DEPS(parentType);
	}
	if (parentExpression != nullptr) {
		UPDATE_DEPS(parentExpression);
	}
}

ir::PrerunValue* PrerunSubEntity::emit(EmitCtx* ctx) {
	SubEntityResult subRes;
	if (skill.has_value()) {
		if (not ctx->has_skill() && (not ctx->has_member_parent() || not ctx->get_member_parent()->is_done_skill())) {
			ctx->Error("Could not find an active skill or skill implementation in the current scope", skill.value());
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
	} else if (parentExpression != nullptr) {
		auto exp = parentExpression->emit(ctx);
		if (not exp->get_ir_type()->is_typed()) {
			ctx->Error("Expected a type here, but found an expression of type " +
			               ctx->color(exp->get_ir_type()->to_string()) + ". Expressions cannot have children entities",
			           parentExpression->fileRange);
		}
		subRes = sub_entity_solver(
		    ctx, true,
		    SubEntityParent::of_type(exp->get_ir_type()->as_typed()->get_subtype(), parentExpression->fileRange), names,
		    fileRange);
	}
	if (subRes.isType) {
		return ir::PrerunValue::get_typed_prerun(ir::TypedType::get((ir::Type*)subRes.data));
	} else {
		return (ir::PrerunValue*)subRes.data;
	}
}

Json PrerunSubEntity::to_json() const {
	Vec<JsonValue> namesJSON;
	for (auto& id : names) {
		namesJSON.push_back(id);
	}
	return Json()
	    ._("nodeType", "prerunSubEntity")
	    ._("hasSkill", skill.has_value())
	    ._("skillRange", skill.has_value() ? skill.value() : JsonValue())
	    ._("names", namesJSON)
	    ._("hasParentType", parentType != nullptr)
	    ._("parentType", parentType ? parentType->to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

String PrerunSubEntity::to_string() const {
	String nameStr;
	for (usize i = 0; i < names.size(); i++) {
		nameStr += ":" + names[i].value;
	}
	return (skill.has_value() ? "skill" : (parentType->to_string())) + nameStr;
}

} // namespace qat::ast
