#include "./text.hpp"
#include "../../IR/logic.hpp"
#include "../context.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

TextType::TextType(ir::Ctx* irCtx, bool _isPacked) : isPack(_isPacked) {
	linkingName = "qat'text" + String(isPack ? ":[pack]" : "");
	if (llvm::StructType::getTypeByName(irCtx->llctx, linkingName)) {
		llvmType = llvm::StructType::getTypeByName(irCtx->llctx, linkingName);
	} else {
		llvmType = llvm::StructType::create(
		    {
		        llvm::PointerType::get(llvm::Type::getInt8Ty(irCtx->llctx), 0U),
		        llvm::Type::getIntNTy(irCtx->llctx,
		                              irCtx->clangTargetInfo->getTypeWidth(irCtx->clangTargetInfo->getSizeType())),
		    },
		    linkingName, isPack);
	}
}

ir::PrerunValue* TextType::create_value(ir::Ctx* irCtx, ir::Mod* mod, String value) {
	auto strTy = ir::TextType::get(irCtx);
	return ir::PrerunValue::get(
	    llvm::ConstantStruct::get(llvm::cast<llvm::StructType>(strTy->get_llvm_type()),
	                              {irCtx->builder.CreateGlobalStringPtr(
	                                   value, irCtx->get_global_string_name(),
	                                   irCtx->dataLayout.getDefaultGlobalsAddressSpace(), mod->get_llvm_module()),
	                               llvm::ConstantInt::get(llvm::Type::getInt64Ty(irCtx->llctx), value.length())}),
	    strTy);
}

bool TextType::is_packed() const { return isPack; }

bool TextType::is_type_sized() const { return true; }

TextType* TextType::get(ir::Ctx* irCtx, bool isPacked) {
	for (auto* typ : allTypes) {
		if (typ->type_kind() == TypeKind::TEXT && (((TextType*)typ)->isPack = isPacked)) {
			return (TextType*)typ;
		}
	}
	return std::construct_at(OwnNormal(TextType), irCtx, isPacked);
}

String TextType::value_to_string(ir::PrerunValue* value) {
	auto* initial =
	    llvm::cast<llvm::ConstantDataArray>(value->get_llvm_constant()->getAggregateElement(0u)->getOperand(0u));
	if (initial->getNumElements() == 1u) {
		return "";
	} else {
		auto tempStr = String(initial->getRawDataValues().data(), initial->getRawDataValues().size());
		if (tempStr[tempStr.size() - 1] == '\0') {
			return tempStr.substr(0, tempStr.size() - 1);
		}
		return tempStr;
	}
}

Maybe<String> TextType::to_prerun_generic_string(ir::PrerunValue* val) const {
	auto* initial =
	    llvm::cast<llvm::ConstantDataArray>(val->get_llvm_constant()->getAggregateElement(0u)->getOperand(0u));
	if (initial->getNumElements() == 1u) {
		return "\"\"";
	} else {
		auto tempStr = initial->getRawDataValues().str();
		if (tempStr[tempStr.size() - 1] == '\0') {
			return '"' + tempStr.substr(0, tempStr.size() - 1) + '"';
		}
		return '"' + tempStr + '"';
	}
}

Maybe<bool> TextType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	return ir::Logic::compare_prerun_text(first->get_llvm_constant()->getAggregateElement(0u),
	                                      first->get_llvm_constant()->getAggregateElement(1u),
	                                      second->get_llvm_constant()->getAggregateElement(0u),
	                                      second->get_llvm_constant()->getAggregateElement(1u), irCtx->llctx);
}

TypeKind TextType::type_kind() const { return TypeKind::TEXT; }

String TextType::to_string() const { return "text" + String(isPack ? ":[pack]" : ""); }

} // namespace qat::ir
