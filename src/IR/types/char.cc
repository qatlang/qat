#include "./char.hpp"
#include "../../utils/utils.hpp"
#include "../value.hpp"

#include <llvm/IR/Type.h>

namespace qat::ir {

CharType::CharType(llvm::LLVMContext& llctx) {
	llvmType    = llvm::cast<llvm::Type>(llvm::Type::getIntNTy(llctx, 21u));
	linkingName = "qat'char";
}

Maybe<String> CharType::to_prerun_generic_string(ir::PrerunValue* val) const {
	String str;
	str.reserve(4);
	auto utfRes = utils::unicode_scalar_to_utf8(
	    static_cast<u32>(*llvm::cast<llvm::ConstantInt>(val->get_llvm_constant())->getValue().getRawData()));
	if (not utfRes.has_value()) {
		return None;
	}
	for (u8 i = 0; i < utfRes->second; i++) {
		str += utfRes->first[i];
	}
	return "`" + str + "`";
}

Maybe<bool> CharType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	return llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(
	    llvm::CmpInst::ICMP_EQ, first->get_llvm_constant(), second->get_llvm_constant()));
}

} // namespace qat::ir
