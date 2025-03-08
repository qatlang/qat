
#include "./float.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../value.hpp"
#include "./native_type.hpp"

#include <llvm/Analysis/ConstantFolding.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/raw_ostream.h>

namespace qat::ir {

FloatType::FloatType(FloatTypeKind _kind, llvm::LLVMContext& ctx) : kind(_kind) {
	switch (kind) {
		case FloatTypeKind::_brain: {
			llvmType = llvm::Type::getBFloatTy(ctx);
			break;
		}
		case FloatTypeKind::_16: {
			llvmType = llvm::Type::getHalfTy(ctx);
			break;
		}
		case FloatTypeKind::_32: {
			llvmType = llvm::Type::getFloatTy(ctx);
			break;
		}
		case FloatTypeKind::_64: {
			llvmType = llvm::Type::getDoubleTy(ctx);
			break;
		}
		case FloatTypeKind::_80: {
			llvmType = llvm::Type::getX86_FP80Ty(ctx);
			break;
		}
		case FloatTypeKind::_128PPC: {
			llvmType = llvm::Type::getPPC_FP128Ty(ctx);
			break;
		}
		case FloatTypeKind::_128: {
			llvmType = llvm::Type::getFP128Ty(ctx);
			break;
		}
	}
	linkingName = "qat'" + to_string();
}

FloatType* FloatType::get(FloatTypeKind _kind, llvm::LLVMContext& llctx) {
	for (auto* typ : allTypes) {
		if (typ->is_float()) {
			if (typ->as_float()->get_float_kind() == _kind) {
				return typ->as_float();
			}
		}
	}
	return std::construct_at(OwnNormal(FloatType), _kind, llctx);
}

PrerunValue* FloatType::get_prerun_default_value(ir::Ctx* irCtx) {
	return ir::PrerunValue::get(llvm::ConstantFP::getZero(llvmType), this);
}

TypeKind FloatType::type_kind() const { return TypeKind::FLOAT; }

FloatTypeKind FloatType::get_float_kind() const { return kind; }

bool FloatType::is_type_sized() const { return true; }

String FloatType::to_string() const {
	switch (kind) {
		case FloatTypeKind::_brain: {
			return "fbrain";
		}
		case FloatTypeKind::_16: {
			return "f16";
		}
		case FloatTypeKind::_32: {
			return "f32";
		}
		case FloatTypeKind::_64: {
			return "f64";
		}
		case FloatTypeKind::_80: {
			return "f80";
		}
		case FloatTypeKind::_128: {
			return "f128";
		}
		case FloatTypeKind::_128PPC: {
			return "f128ppc";
		}
	}
}

Maybe<String> FloatType::to_prerun_generic_string(ir::PrerunValue* val) const {
	if (val->get_ir_type()->is_float()) {
		String                   strRef;
		llvm::raw_string_ostream stream(strRef);
		val->get_llvm_constant()->printAsOperand(stream);
		return strRef.find(' ') != String::npos ? strRef.substr(strRef.find(' ') + 1) : strRef;
	} else {
		return None;
	}
}

Maybe<bool> FloatType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (first->get_ir_type()->is_same(second->get_ir_type()) && first->get_ir_type()->is_float()) {
		return llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(llvm::CmpInst::FCMP_OEQ,
		                                                                          first->get_llvm_constant(),
		                                                                          second->get_llvm_constant()))
		    ->getValue()
		    .getBoolValue();
	} else {
		return None;
	}
}

} // namespace qat::ir
