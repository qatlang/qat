#include "./typed.hpp"
#include "../../utils/qat_region.hpp"
#include "../context.hpp"
#include "../type_id.hpp"
#include "../value.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/ConstantFold.h>
#include <llvm/IR/Type.h>

namespace qat::ir {

TypedType* TypedType::typedType = nullptr;

TypedType::TypedType(ir::Ctx* _ctx) {
	ctx         = _ctx;
	llvmType    = llvm::PointerType::get(ctx->llctx, ctx->dataLayout.getDefaultGlobalsAddressSpace());
	linkingName = "qat'type";
	typedType   = this;
}

TypedType* TypedType::get(ir::Ctx* ctx) {
	if (typedType) {
		return typedType;
	}
	return std::construct_at(OwnNormal(TypedType), ctx);
}

Maybe<bool> TypedType::equality_of(ir::Ctx*, ir::PrerunValue* first, ir::PrerunValue* second) const {
	return TypeInfo::get_for(first->get_llvm_constant())
	    ->type->is_same(TypeInfo::get_for(second->get_llvm_constant())->type);
}

Maybe<String> TypedType::to_prerun_generic_string(ir::PrerunValue* val) const {
	return TypeInfo::get_for(val->get_llvm_constant())->type->to_string();
}

} // namespace qat::ir
