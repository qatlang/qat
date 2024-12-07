#include "./internal.hpp"
#include "./context.hpp"
#include "./types/function.hpp"
#include "./types/native_type.hpp"
#include "./types/pointer.hpp"
#include "./types/void.hpp"

namespace qat::ir {

FunctionType* Internal::printf_signature(Ctx* irCtx) {
	return FunctionType::create(
		ReturnType::get(NativeType::get_int(irCtx)),
		{ArgumentType::create_normal(NativeType::get_cstr(irCtx), None, false), ArgumentType::create_variadic(None)},
		irCtx->llctx);
}

FunctionType* Internal::malloc_signature(Ctx* irCtx) {
	return FunctionType::create(ReturnType::get(MarkType::get(true, VoidType::get(irCtx->llctx), false,
															  MarkOwner::of_anonymous(), false, irCtx)),
								{ArgumentType::create_normal(NativeType::get_usize(irCtx), None, false)}, irCtx->llctx);
}

FunctionType* Internal::realloc_signature(Ctx* irCtx) {
	auto markTy = MarkType::get(true, VoidType::get(irCtx->llctx), false, MarkOwner::of_anonymous(), false, irCtx);
	return FunctionType::create(ReturnType::get(markTy),
								{ArgumentType::create_normal(markTy, None, false),
								 ArgumentType::create_normal(NativeType::get_usize(irCtx), None, false)},
								irCtx->llctx);
}

FunctionType* Internal::free_signature(Ctx* irCtx) {
	return FunctionType::create(ReturnType::get(VoidType::get(irCtx->llctx)),
								{ArgumentType::create_normal(MarkType::get(true, VoidType::get(irCtx->llctx), false,
																		   MarkOwner::of_anonymous(), false, irCtx),
															 None, false)},
								irCtx->llctx);
}

} // namespace qat::ir
