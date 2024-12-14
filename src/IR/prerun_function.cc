#include "./prerun_function.hpp"
#include "../ast/emit_ctx.hpp"
#include "../ast/prerun_sentences/internal_exceptions.hpp"
#include "../ast/prerun_sentences/prerun_sentence.hpp"
#include "./context.hpp"

namespace qat::ir {

PreBlock::PreBlock(PrerunCallState* _callState, PreBlock* _parent) : callState(_callState), parent(_parent) {
	if (parent && parent->firstChild == nullptr) {
		parent->firstChild = this;
	}
}

void PreBlock::make_active() { callState->activeBlock = this; }

bool PrerunCallState::has_arg_with_name(String const& name) {
	for (auto arg : function->argTypes) {
		if (arg->get_name() == name) {
			return true;
		}
	}
	return false;
}

PrerunValue* PrerunCallState::get_arg_value_for(String const& name) {
	for (usize i = 0; i < function->argTypes.size(); i++) {
		if (function->argTypes[i]->get_name() == name) {
			return argumentValues[i];
		}
	}
	return nullptr;
}

PrerunFunction::PrerunFunction(Mod* _parent, Identifier _name, Type* _retTy, Vec<ArgumentType*> _argTys,
                               Pair<Vec<ast::PrerunSentence*>, FileRange> _sentences, VisibilityInfo visib,
                               llvm::LLVMContext& ctx)
    : PrerunValue((llvm::Constant*)this, new ir::FunctionType(ReturnType::get(_retTy), _argTys, ctx)),
      EntityOverview("prerunFunction", Json(), _name.range), name(_name), returnType(_retTy), argTypes(_argTys),
      parent(_parent), visibility(visib), sentences(_sentences) {
	parent->prerunFunctions.push_back(this);
}

String PrerunFunction::get_full_name() const { return parent->get_fullname_with_child(name.value); }

PrerunValue* PrerunFunction::call_prerun(Vec<PrerunValue*> argValues, Ctx* irCtx, FileRange fileRange) {
	auto callState = PrerunCallState::get(this, argValues);
	auto emitCtx   = ast::EmitCtx::get(irCtx, parent)->with_prerun_call_state(callState);
	try {
		for (auto* snt : sentences.first) {
			snt->emit(emitCtx);
		}
	} catch (InternalPrerunGive& given) {
		std::destroy_at(callState);
		return given.value;
	} catch (...) {
		irCtx->Error("Failed to call the prerun function with an exception", fileRange);
	}
	if (returnType->is_void()) {
		std::destroy_at(callState);
		return nullptr;
	} else {
		irCtx->Error("This prerun function did not give any value", fileRange);
	}
}

void PrerunFunction::update_overview() {
	Vec<JsonValue> argTyJSON;
	for (auto* argTy : argTypes) {
		argTyJSON.push_back(argTy->to_json());
	}
	ovInfo = Json()
	             ._("name", name)
	             ._("fullName", get_full_name())
	             ._("arguments", argTyJSON)
	             ._("parent", parent->get_id())
	             ._("givenType", returnType->get_id())
	             ._("visibility", visibility);
}

} // namespace qat::ir
