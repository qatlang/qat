#include "./tuple.hpp"
#include "./pointer.hpp"
#include "reference.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

TupleType::TupleType(Vec<Type*> _types, bool _isPacked, llvm::LLVMContext& llctx)
	: subTypes(std::move(_types)), isPacked(_isPacked) {
	Vec<llvm::Type*> subTypesLLVM;
	for (auto* typ : subTypes) {
		subTypesLLVM.push_back(typ->get_llvm_type());
	}
	llvmType	= llvm::StructType::get(llctx, subTypesLLVM, isPacked);
	linkingName = "qat'tuple:[" + String(isPacked ? "pack," : "");
	for (usize i = 0; i < subTypes.size(); i++) {
		linkingName += subTypes.at(i)->get_name_for_linking();
		if (i != (subTypes.size() - 1)) {
			linkingName += ",";
		}
	}
	linkingName += "]";
}

bool TupleType::is_copy_constructible() const {
	for (auto* sub : subTypes) {
		if (!sub->is_copy_constructible()) {
			return false;
		}
	}
	return true;
}

bool TupleType::is_copy_assignable() const {
	for (auto* sub : subTypes) {
		if (!sub->is_copy_assignable()) {
			return false;
		}
	}
	return true;
}

bool TupleType::is_move_constructible() const {
	for (auto* sub : subTypes) {
		if (!sub->is_move_constructible()) {
			return false;
		}
	}
	return true;
}

bool TupleType::is_move_assignable() const {
	for (auto* sub : subTypes) {
		if (!sub->is_move_assignable()) {
			return false;
		}
	}
	return true;
}

bool TupleType::is_destructible() const {
	for (auto* sub : subTypes) {
		if (!sub->is_destructible()) {
			return false;
		}
	}
	return true;
}

void TupleType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (is_trivially_copyable()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
	} else {
		if (is_copy_constructible()) {
			for (usize i = 0; i < subTypes.size(); i++) {
				auto* subTy = subTypes.at(i);
				subTy->copy_construct_value(
					irCtx,
					ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, first->get_llvm(), i),
								   ir::ReferenceType::get(true, subTy, irCtx), false),
					ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), i),
								   ir::ReferenceType::get(false, subTy, irCtx), false),
					fun);
			}
		} else {
			irCtx->Error("Could not copy construct an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

void TupleType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (is_trivially_copyable()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
	} else {
		if (is_copy_assignable()) {
			for (usize i = 0; i < subTypes.size(); i++) {
				auto* subTy = subTypes.at(i);
				subTy->copy_assign_value(irCtx,
										 ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, first->get_llvm(), i),
														ir::ReferenceType::get(true, subTy, irCtx), false),
										 ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), i),
														ir::ReferenceType::get(false, subTy, irCtx), false),
										 fun);
			}
		} else {
			irCtx->Error("Could not copy assign an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

void TupleType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (is_trivially_movable()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), second->get_llvm());
	} else {
		if (is_move_constructible()) {
			for (usize i = 0; i < subTypes.size(); i++) {
				auto* subTy = subTypes.at(i);
				subTy->move_construct_value(
					irCtx,
					ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, first->get_llvm(), i),
								   ir::ReferenceType::get(true, subTy, irCtx), false),
					ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), i),
								   ir::ReferenceType::get(false, subTy, irCtx), false),
					fun);
			}
		} else {
			irCtx->Error("Could not move construct an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

void TupleType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (is_trivially_movable()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(llvmType, second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), second->get_llvm());
	} else {
		if (is_move_assignable()) {
			for (usize i = 0; i < subTypes.size(); i++) {
				auto* subTy = subTypes.at(i);
				subTy->move_assign_value(irCtx,
										 ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, first->get_llvm(), i),
														ir::ReferenceType::get(true, subTy, irCtx), false),
										 ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, second->get_llvm(), i),
														ir::ReferenceType::get(false, subTy, irCtx), false),
										 fun);
			}
		} else {
			irCtx->Error("Could not move assign an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

void TupleType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (is_trivially_movable()) {
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(llvmType), instance->get_llvm());
	} else {
		if (is_destructible()) {
			for (usize i = 0; i < subTypes.size(); i++) {
				auto* subTy = subTypes.at(i);
				subTy->destroy_value(irCtx,
									 ir::Value::get(irCtx->builder.CreateStructGEP(llvmType, instance->get_llvm(), i),
													ir::ReferenceType::get(false, subTy, irCtx), false),
									 fun);
			}
		} else {
			irCtx->Error("Could not destroy an instance of type " + irCtx->color(to_string()), None);
		}
	}
}

TupleType* TupleType::get(Vec<Type*> newSubTypes, bool isPacked, llvm::LLVMContext& llctx) {
	for (auto* typ : allTypes) {
		if (typ->is_tuple()) {
			auto subTys	 = typ->as_tuple()->getSubTypes();
			bool is_same = true;
			if (typ->as_tuple()->isPackedTuple() != isPacked) {
				is_same = false;
			} else if (newSubTypes.size() != subTys.size()) {
				is_same = false;
			} else {
				for (usize i = 0; i < subTys.size(); i++) {
					if (!subTys.at(i)->is_same(newSubTypes.at(i))) {
						is_same = false;
						break;
					}
				}
			}
			if (is_same) {
				return typ->as_tuple();
			}
		}
	}
	return std::construct_at(OwnNormal(TupleType), newSubTypes, isPacked, llctx);
}

Vec<Type*> TupleType::getSubTypes() const { return subTypes; }

Type* TupleType::getSubtypeAt(u32 index) { return subTypes.at(index); }

u32 TupleType::getSubTypeCount() const { return subTypes.size(); }

bool TupleType::isPackedTuple() const { return isPacked; }

bool TupleType::is_type_sized() const { return !subTypes.empty(); }

TypeKind TupleType::type_kind() const { return TypeKind::tuple; }

String TupleType::to_string() const {
	String result = String(isPacked ? "pack " : "") + "(";
	for (usize i = 0; i < subTypes.size(); i++) {
		result += subTypes.at(i)->to_string();
		if (i != (subTypes.size() - 1)) {
			result += "; ";
		}
	}
	result += ")";
	return result;
}

} // namespace qat::ir
