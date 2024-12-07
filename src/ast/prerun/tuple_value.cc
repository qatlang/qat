#include "./tuple_value.hpp"

namespace qat::ast {

void PrerunTupleValue::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> dep, ir::EntityState* ent,
										   EmitCtx* ctx) {
	for (auto mem : members) {
		mem->update_dependencies(phase, ir::DependType::complete, ent, ctx);
	}
}

ir::PrerunValue* PrerunTupleValue::emit(EmitCtx* ctx) {
	Maybe<ir::TupleType*> expected;
	if (is_type_inferred()) {
		if (!inferredType->is_tuple()) {
			ctx->Error("This expression should be of a tuple type, but the type inferred from scope is " +
						   ctx->color(inferredType->to_string()),
					   fileRange);
		}
		if (members.size() != inferredType->as_tuple()->getSubTypeCount()) {
			ctx->Error("The type inferred from scope for this expression is " + ctx->color(inferredType->to_string()) +
						   " which expects " + ctx->color(std::to_string(inferredType->as_tuple()->getSubTypeCount())) +
						   " values. But " +
						   ((inferredType->as_tuple()->getSubTypeCount() > members.size()) ? "only " : "") +
						   std::to_string(members.size()) + " values were provided",
					   fileRange);
		}
	}
	Vec<ir::PrerunValue*> memberVals;
	for (usize i = 0; i < members.size(); i++) {
		if (expected.has_value()) {
			if (members.at(i)->has_type_inferrance()) {
				members.at(i)->as_type_inferrable()->set_inference_type(expected.value()->getSubtypeAt(i));
			}
		}
		memberVals.push_back(members.at(i)->emit(ctx));
		if (expected.has_value() && !expected.value()->getSubtypeAt(i)->is_same(memberVals.back()->get_ir_type())) {
			ctx->Error("The tuple type inferred is " + ctx->color(expected.value()->to_string()) +
						   " so the expected type of this expression is " +
						   ctx->color(expected.value()->getSubtypeAt(i)->to_string()) +
						   " but got an expression of type " +
						   ctx->color(memberVals.back()->get_ir_type()->to_string()),
					   members.at(i)->fileRange);
		}
	}
	Vec<ir::Type*>		 memTys;
	Vec<llvm::Constant*> memConsts;
	for (auto mem : memberVals) {
		memTys.push_back(mem->get_ir_type());
		memConsts.push_back(mem->get_llvm_constant());
	}
	if (!expected.has_value()) {
		// FIXME - Support packing
		expected = ir::TupleType::get(memTys, false, ctx->irCtx->llctx);
	}
	return ir::PrerunValue::get(llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(expected.value()), memConsts),
								expected.value());
}

String PrerunTupleValue::to_string() const {
	String result("(");
	for (usize i = 0; i < members.size(); i++) {
		result += members[i]->to_string();
		if ((members.size() == 1) || (i != (members.size() - 1))) {
			result += ";";
		}
	}
	result += ")";
	return result;
}

Json PrerunTupleValue::to_json() const {
	Vec<JsonValue> memsJson;
	for (auto mem : members) {
		memsJson.push_back(mem->to_json());
	}
	return Json()._("nodeType", "prerunTupleValue")._("values", memsJson)._("fileRange", fileRange);
}

} // namespace qat::ast