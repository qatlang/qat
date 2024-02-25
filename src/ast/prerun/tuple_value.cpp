#include "./tuple_value.hpp"

namespace qat::ast {

void PrerunTupleValue::update_dependencies(IR::EmitPhase phase, Maybe<IR::DependType> dep, IR::EntityState* ent,
                                           IR::Context* ctx) {
  for (auto mem : members) {
    mem->update_dependencies(phase, IR::DependType::complete, ent, ctx);
  }
}

IR::PrerunValue* PrerunTupleValue::emit(IR::Context* ctx) {
  Maybe<IR::TupleType*> expected;
  if (isTypeInferred()) {
    if (!inferredType->isTuple()) {
      ctx->Error("This expression should be of a tuple type, but the type inferred from scope is " +
                     ctx->highlightError(inferredType->toString()),
                 fileRange);
    }
    if (members.size() != inferredType->asTuple()->getSubTypeCount()) {
      ctx->Error("The type inferred from scope for this expression is " +
                     ctx->highlightError(inferredType->toString()) + " which expects " +
                     ctx->highlightError(std::to_string(inferredType->asTuple()->getSubTypeCount())) + " values. But " +
                     ((inferredType->asTuple()->getSubTypeCount() > members.size()) ? "only " : "") +
                     std::to_string(members.size()) + " values were provided",
                 fileRange);
    }
  }
  Vec<IR::PrerunValue*> memberVals;
  for (usize i = 0; i < members.size(); i++) {
    if (expected.has_value()) {
      if (members.at(i)->hasTypeInferrance()) {
        members.at(i)->asTypeInferrable()->setInferenceType(expected.value()->getSubtypeAt(i));
      }
    }
    memberVals.push_back(members.at(i)->emit(ctx));
    if (expected.has_value() && !expected.value()->getSubtypeAt(i)->isSame(memberVals.back()->getType())) {
      ctx->Error("The tuple type inferred is " + ctx->highlightError(expected.value()->toString()) +
                     " so the expected type of this expression is " +
                     ctx->highlightError(expected.value()->getSubtypeAt(i)->toString()) +
                     " but got an expression of type " + ctx->highlightError(memberVals.back()->getType()->toString()),
                 members.at(i)->fileRange);
    }
  }
  Vec<IR::QatType*>    memTys;
  Vec<llvm::Constant*> memConsts;
  for (auto mem : memberVals) {
    memTys.push_back(mem->getType());
    memConsts.push_back(mem->getLLVMConstant());
  }
  if (!expected.has_value()) {
    // FIXME - Support packing
    expected = IR::TupleType::get(memTys, false, ctx->llctx);
  }
  return new IR::PrerunValue(llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(expected.value()), memConsts),
                             expected.value());
}

String PrerunTupleValue::toString() const {
  String result("(");
  for (usize i = 0; i < members.size(); i++) {
    result += members[i]->toString();
    if ((members.size() == 1) || (i != (members.size() - 1))) {
      result += ";";
    }
  }
  result += ")";
  return result;
}

Json PrerunTupleValue::toJson() const {
  Vec<JsonValue> memsJson;
  for (auto mem : members) {
    memsJson.push_back(mem->toJson());
  }
  return Json()._("nodeType", "prerunTupleValue")._("values", memsJson)._("fileRange", fileRange);
}

} // namespace qat::ast