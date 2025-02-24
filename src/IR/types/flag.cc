#include "./flag.hpp"
#include "../qat_module.hpp"
#include "./unsigned.hpp"

#include <llvm/IR/ConstantFold.h>

namespace qat::ir {

FlagType::FlagType(Identifier _name, Mod* _parent, Vec<FlagVariant> _variants, Maybe<Vec<PrerunValue*>> _values,
                   UnsignedType* _underlyingType, FileRange _range, VisibilityInfo _visibility)
    : name(std::move(_name)), parent(_parent), variants(std::move(_variants)), values(std::move(_values)),
      underlyingType(_underlyingType), range(std::move(_range)), visibility(std::move(_visibility)) {
	for (auto& it : variants) {
		if (it.isDefault) {
			hasDefaultVariants = true;
		}
	}
	if (parent) {
		parent->flagTypes.push_back(this);
	}
}

String FlagType::get_full_name() const { return parent->get_fullname_with_child(name.value); }

PrerunValue* FlagType::get_prerun_default_value(ir::Ctx* irCtx) {
	if (hasDefaultVariants) {
		String defValStr = "";
		for (auto i = 0; i < variants.size(); i++) {
			if (variants[i].isDefault) {
				defValStr += "1";
			} else {
				defValStr += "0";
			}
		}
		return ir::PrerunValue::get(
		    llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(underlyingType->get_llvm_type()), defValStr, 2u),
		    underlyingType);
	} else {
		return ir::PrerunValue::get(llvm::ConstantInt::get(underlyingType->get_llvm_type(), 0u, false), underlyingType);
	}
}

Maybe<String> FlagType::to_prerun_generic_string(ir::PrerunValue* val) const {
	auto const llTy   = llvm::cast<llvm::IntegerType>(underlyingType->get_llvm_type());
	auto const intVal = llvm::cast<llvm::ConstantInt>(val->get_llvm_constant());
	String     result = parent->get_fullname_with_child(name.value) + "::{ ";
	for (usize i = 0; i < variants.size(); i++) {
		auto strVal = ((i != 0) ? String(i - 1, '0') : "") + "1" +
		              ((i != (variants.size() - 1)) ? String(variants.size() - 1 - i, '0') : "");
		auto const itemVal = llvm::ConstantInt::get(llTy, strVal, 2u);
		if (llvm::cast<llvm::ConstantInt>(
		        llvm::ConstantFoldCompareInstruction(
		            llvm::CmpInst::ICMP_EQ,
		            llvm::ConstantFoldBinaryInstruction(llvm::Instruction::BinaryOps::And, intVal, itemVal), itemVal))
		        ->getValue()
		        .getBoolValue()) {
			if (i != 0) {
				result += " + ";
			}
			result += variants[i].names.front().value;
		}
	}
	result += " }";
	return result;
}

Maybe<usize> FlagType::get_index_of(String name) const {
	for (usize i = 0; i < variants.size(); i++) {
		for (auto& id : variants[i].names) {
			if (id.value == name) {
				return i;
			}
		}
	}
	return None;
}

Maybe<bool> FlagType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	return llvm::cast<llvm::ConstantInt>(llvm::ConstantFoldCompareInstruction(llvm::CmpInst::ICMP_EQ,
	                                                                          first->get_llvm_constant(),
	                                                                          second->get_llvm_constant()))
	    ->getValue()
	    .getBoolValue();
}

bool FlagType::has_value_for(String name) const {
	for (usize i = 0; i < variants.size(); i++) {
		for (auto& id : variants[i].names) {
			if (id.value == name) {
				return true;
			}
		}
	}
	return false;
}

PrerunValue* FlagType::get_value_for(String name) const {
	for (usize i = 0; i < variants.size(); i++) {
		for (auto& id : variants[i].names) {
			if (id.value == name) {
				if (values.has_value()) {
					return values.value().at(i);
				} else {
					auto const bits = underlyingType->get_bitwidth();
					String     bitString =
                        (i == 0) ? (String("1") + String(bits - 1, '0'))
					                 : (String(i - 1, '0') + '1' +
                                    ((i < (variants.size() - 1)) ? String(variants.size() - 1 - i, '0') : ""));
					return ir::PrerunValue::get(
					    llvm::ConstantInt::get(llvm::cast<llvm::IntegerType>(underlyingType->get_llvm_type()),
					                           bitString, 2u),
					    underlyingType);
				}
			}
		}
	}
	return nullptr;
}

String FlagType::to_string() const { return parent->get_fullname_with_child(name.value); }

} // namespace qat::ir
