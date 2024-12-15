#include "./mix.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../control_flow.hpp"
#include "../generics.hpp"
#include "../qat_module.hpp"
#include "./reference.hpp"
#include "./type_kind.hpp"

#include <algorithm>
#include <cmath>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

#define TWO_POWER_4  16ULL
#define TWO_POWER_8  256ULL
#define TWO_POWER_16 65536ULL
#define TWO_POWER_32 4294967296ULL

MixType::MixType(Identifier _name, ir::OpaqueType* _opaquedTy, Vec<GenericArgument*> _generics, Mod* _parent,
                 Vec<Pair<Identifier, Maybe<Type*>>> _subtypes, Maybe<usize> _defaultVal, ir::Ctx* irCtx,
                 bool _isPacked, const VisibilityInfo& _visibility, FileRange _fileRange, Maybe<MetaInfo> _metaInfo)
    : ExpandedType(std::move(_name), std::move(_generics), _parent, _visibility),
      EntityOverview("mixType", Json(), _name.range), subtypes(std::move(_subtypes)), isPack(_isPacked),
      defaultVal(_defaultVal), fileRange(std::move(_fileRange)), metaInfo(_metaInfo), opaquedType(_opaquedTy) {
	for (const auto& sub : subtypes) {
		if (sub.second.has_value()) {
			auto* typ = sub.second.value();
			if (!typ->is_trivially_copyable()) {
				isTrivialCopy = false;
			}
			if (!typ->is_trivially_movable()) {
				isTrivialMove = false;
			}
			SHOW("Getting size of the subtype in SUM TYPE")
			usize size =
			    typ->is_opaque()
			        ? ((typ->as_opaque()->has_subtype() && typ->as_opaque()->get_subtype()->is_type_sized())
			               ? irCtx->dataLayout->getTypeAllocSizeInBits(typ->as_opaque()->get_subtype()->get_llvm_type())
			               : typ->as_opaque()->get_deduced_size())
			        : irCtx->dataLayout->getTypeAllocSizeInBits(typ->get_llvm_type());
			SHOW("Got size " << size << " of subtype named " << sub.first.value)
			if (size > maxSize) {
				maxSize = size;
			}
		}
	}
	findTagBitWidth();
	SHOW("Opaqued type is: " << opaquedType)
	SHOW("Tag bitwidth is " << tagBitWidth)
	linkingName = opaquedType->get_name_for_linking();
	llvmType    = opaquedType->get_llvm_type();
	llvm::cast<llvm::StructType>(llvmType)->setBody(
	    {llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), llvm::Type::getIntNTy(irCtx->llctx, maxSize)}, _isPacked);
	if (parent) {
		parent->mixTypes.push_back(this);
	}
	opaquedType->set_sub_type(this);
	ovInfo            = opaquedType->ovInfo;
	ovRange           = opaquedType->ovRange;
	ovMentions        = opaquedType->ovMentions;
	ovBroughtMentions = opaquedType->ovBroughtMentions;
}

LinkNames MixType::get_link_names() const {
	Maybe<String> foreignID;
	Maybe<String> linkAlias;
	if (metaInfo) {
		foreignID = metaInfo->get_foreign_id();
		linkAlias = metaInfo->get_value_as_string_for(ir::MetaInfo::linkAsKey);
	}
	if (!foreignID.has_value()) {
		foreignID = parent->get_relevant_foreign_id();
	}
	auto linkNames = parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::mix), foreignID);
	if (is_generic()) {
		Vec<LinkNames> genericlinkNames;
		for (auto* param : generics) {
			if (param->is_typed()) {
				genericlinkNames.push_back(
				    LinkNames({LinkNameUnit(param->as_typed()->get_type()->get_name_for_linking(),
				                            LinkUnitType::genericTypeValue)},
				              None, nullptr));
			} else if (param->is_prerun()) {
				auto* preRes = param->as_prerun();
				genericlinkNames.push_back(LinkNames(
				    {LinkNameUnit(preRes->get_type()->to_prerun_generic_string(preRes->get_expression()).value(),
				                  LinkUnitType::genericPrerunValue)},
				    None, nullptr));
			}
		}
		linkNames.addUnit(LinkNameUnit("", LinkUnitType::genericList, genericlinkNames), None);
	}
	linkNames.setLinkAlias(linkAlias);
	return linkNames;
}

void MixType::update_overview() {
	Vec<JsonValue> subTyJson;
	for (auto const& sub : subtypes) {
		subTyJson.push_back(Json()
		                        ._("name", sub.first)
		                        ._("hasType", sub.second.has_value())
		                        ._("typeID", sub.second.has_value() ? sub.second.value()->get_id() : JsonValue())
		                        ._("type", sub.second.has_value() ? sub.second.value()->to_string() : JsonValue()));
	}
	ovInfo._("typeID", get_id())
	    ._("fullName", get_full_name())
	    ._("moduleID", parent->get_id())
	    ._("tagBitWidth", tagBitWidth)
	    ._("isPacked", isPack)
	    ._("hasDefaultValue", defaultVal.has_value())
	    ._("subTypes", subTyJson)
	    ._("visibility", visibility);
	if (has_default_variant()) {
		ovInfo._("defaultValue", defaultVal.value());
	}
}

void MixType::findTagBitWidth() {
	tagBitWidth = 1;
	while (std::pow(2, tagBitWidth) <= (subtypes.size() + 1)) {
		tagBitWidth++;
	}
}

usize MixType::get_index_of(const String& name) const {
	for (usize i = 0; i < subtypes.size(); i++) {
		if (subtypes.at(i).first.value == name) {
			return i + 1;
		}
	}
	// NOLINTNEXTLINE(clang-diagnostic-return-type)
}

bool MixType::has_default_variant() const { return defaultVal.has_value(); }

usize MixType::get_default_index() const { return defaultVal.value_or(0u); }

Pair<bool, bool> MixType::has_variant_with_name(const String& sname) const {
	for (const auto& sTy : subtypes) {
		if (sTy.first.value == sname) {
			return {true, sTy.second.has_value()};
		}
	}
	return {false, false};
}

Type* MixType::get_variant_with_name(const String& sname) const {
	for (const auto& sTy : subtypes) {
		if (sTy.first.value == sname) {
			return sTy.second.value();
		}
	}
	return nullptr;
}

void MixType::get_missing_names(Vec<Identifier>& vals, Vec<Identifier>& missing) const {
	for (const auto& sub : subtypes) {
		bool result = false;
		for (const auto& val : vals) {
			if (sub.first.value == val.value) {
				result = true;
				break;
			}
		}
		if (!result) {
			missing.push_back(sub.first);
		}
	}
}

usize MixType::get_variant_count() const { return subtypes.size(); }

bool MixType::is_packed() const { return isPack; }

usize MixType::get_tag_bitwidth() const { return tagBitWidth; }

u64 MixType::get_data_bitwidth() const { return maxSize; }

FileRange MixType::get_file_range() const { return fileRange; }

bool MixType::is_type_sized() const { return true; }

bool MixType::is_trivially_copyable() const { return isTrivialCopy; }

bool MixType::is_trivially_movable() const { return isTrivialMove; }

bool MixType::is_copy_constructible() const {
	for (auto sub : subtypes) {
		if (sub.second.has_value() && !sub.second.value()->is_copy_constructible()) {
			return false;
		}
	}
	return true;
}

bool MixType::is_copy_assignable() const {
	for (auto sub : subtypes) {
		if (sub.second.has_value() && !sub.second.value()->is_copy_assignable()) {
			return false;
		}
	}
	return true;
}

bool MixType::is_move_constructible() const {
	for (auto sub : subtypes) {
		if (sub.second.has_value() && !sub.second.value()->is_move_constructible()) {
			return false;
		}
	}
	return true;
}

bool MixType::is_move_assignable() const {
	for (auto sub : subtypes) {
		if (sub.second.has_value() && !sub.second.value()->is_move_assignable()) {
			return false;
		}
	}
	return true;
}

void MixType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (is_copy_constructible()) {
		auto* prevTag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
		auto*      prevDataPtr = irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u);
		auto*      resDataPtr  = irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u);
		ir::Block* trueBlock   = nullptr;
		ir::Block* falseBlock  = nullptr;
		ir::Block* restBlock   = ir::Block::create(fun, fun->get_block()->get_parent());
		restBlock->link_previous_block(fun->get_block());
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_copy_constructible()) {
				auto* subTy = subtypes.at(i).second.value();
				trueBlock   = ir::Block::create(fun, falseBlock ? falseBlock : fun->get_block());
				falseBlock  = ir::Block::create(fun, falseBlock ? falseBlock : fun->get_block());
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        prevTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    trueBlock->get_bb(), falseBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				subTy->copy_construct_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            resDataPtr, llvm::PointerType::get(subTy->get_llvm_type(),
				                                               irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            prevDataPtr, llvm::PointerType::get(subTy->get_llvm_type(),
				                                                irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(false, subTy, irCtx), false),
				    fun);
				irCtx->builder.CreateStore(
				    llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false),
				    irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				falseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		restBlock->set_active(irCtx->builder);
	} else {
		irCtx->Error("Could not copy construct an instance of type " + irCtx->color(to_string()), None);
	}
}

void MixType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (is_move_constructible()) {
		auto* prevTag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 0u));
		auto*      prevDataPtr = irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), 1u);
		auto*      resDataPtr  = irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 1u);
		ir::Block* trueBlock   = nullptr;
		ir::Block* falseBlock  = nullptr;
		ir::Block* restBlock   = ir::Block::create(fun, fun->get_block()->get_parent());
		restBlock->link_previous_block(fun->get_block());
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_move_constructible()) {
				auto* subTy = subtypes.at(i).second.value();
				trueBlock   = ir::Block::create(fun, falseBlock ? falseBlock : fun->get_block());
				falseBlock  = ir::Block::create(fun, falseBlock ? falseBlock : fun->get_block());
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        prevTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    trueBlock->get_bb(), falseBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				subTy->move_construct_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            resDataPtr, llvm::PointerType::get(subTy->get_llvm_type(),
				                                               irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            prevDataPtr, llvm::PointerType::get(subTy->get_llvm_type(),
				                                                irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(false, subTy, irCtx), false),
				    fun);
				irCtx->builder.CreateStore(
				    llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false),
				    irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), 0u));
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				falseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		restBlock->set_active(irCtx->builder);
	} else {
		irCtx->Error("Could not move construct an instance of type " + irCtx->color(to_string()), None);
	}
}

void MixType::copy_assign_value(ir::Ctx* irCtx, ir::Value* firstInst, ir::Value* secondInst, ir::Function* fun) {
	if (is_copy_assignable()) {
		auto* firstTag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), firstInst->get_llvm(), 0u));
		auto* firstData = irCtx->builder.CreateStructGEP(get_llvm_type(), firstInst->get_llvm(), 1u);
		auto* secondTag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), secondInst->get_llvm(), 0u));
		auto* secondData        = irCtx->builder.CreateStructGEP(get_llvm_type(), firstInst->get_llvm(), 1u);
		auto* sameTagTrueBlock  = ir::Block::create(fun, fun->get_block());
		auto* sameTagFalseBlock = ir::Block::create(fun, fun->get_block());
		auto* restBlock         = ir::Block::create(fun, fun->get_block()->get_parent());
		restBlock->link_previous_block(fun->get_block());
		irCtx->builder.CreateCondBr(irCtx->builder.CreateICmpEQ(firstTag, secondTag), sameTagTrueBlock->get_bb(),
		                            sameTagFalseBlock->get_bb());
		sameTagTrueBlock->set_active(irCtx->builder);
		ir::Block* cmpTrueBlock  = nullptr;
		ir::Block* cmpFalseBlock = nullptr;
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_copy_assignable()) {
				auto* subTy   = subtypes.at(i).second.value();
				cmpTrueBlock  = ir::Block::create(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
				cmpFalseBlock = ir::Block::create(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        firstTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    cmpTrueBlock->get_bb(), cmpFalseBlock->get_bb());
				cmpTrueBlock->set_active(irCtx->builder);
				subTy->copy_assign_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            firstData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                              irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            secondData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                               irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(false, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				cmpFalseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		sameTagFalseBlock->set_active(irCtx->builder);
		ir::Block* firstCmpTrueBlock  = nullptr;
		ir::Block* firstCmpFalseBlock = nullptr;
		ir::Block* firstCmpRestBlock  = ir::Block::create(fun, sameTagFalseBlock);
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_destructible()) {
				auto* subTy       = subtypes.at(i).second.value();
				firstCmpTrueBlock = ir::Block::create(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
				firstCmpFalseBlock =
				    ir::Block::create(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        firstTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    firstCmpTrueBlock->get_bb(), firstCmpFalseBlock->get_bb());
				firstCmpTrueBlock->set_active(irCtx->builder);
				subTy->destroy_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            firstData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                              irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, firstCmpRestBlock->get_bb());
				firstCmpFalseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, firstCmpRestBlock->get_bb());
		firstCmpRestBlock->set_active(irCtx->builder);
		ir::Block* secondCmpTrueBlock  = nullptr;
		ir::Block* secondCmpFalseBlock = nullptr;
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_copy_constructible()) {
				auto* subTy = subtypes.at(i).second.value();
				secondCmpTrueBlock =
				    ir::Block::create(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
				secondCmpFalseBlock =
				    ir::Block::create(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        secondTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    secondCmpTrueBlock->get_bb(), secondCmpFalseBlock->get_bb());
				secondCmpTrueBlock->set_active(irCtx->builder);
				subTy->copy_construct_value(
				    irCtx,

				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            firstData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                              irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            secondData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                               irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(false, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				secondCmpFalseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		restBlock->set_active(irCtx->builder);
	} else {
		irCtx->Error("Could not copy assign an instance of type " + irCtx->color(to_string()), None);
	}
}

void MixType::move_assign_value(ir::Ctx* irCtx, ir::Value* firstInst, ir::Value* secondInst, ir::Function* fun) {
	if (is_move_assignable()) {
		auto* firstTag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), firstInst->get_llvm(), 0u));
		auto* firstData = irCtx->builder.CreateStructGEP(get_llvm_type(), firstInst->get_llvm(), 1u);
		auto* secondTag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), secondInst->get_llvm(), 0u));
		auto* secondData        = irCtx->builder.CreateStructGEP(get_llvm_type(), firstInst->get_llvm(), 1u);
		auto* sameTagTrueBlock  = ir::Block::create(fun, fun->get_block());
		auto* sameTagFalseBlock = ir::Block::create(fun, fun->get_block());
		auto* restBlock         = ir::Block::create(fun, fun->get_block()->get_parent());
		restBlock->link_previous_block(fun->get_block());
		irCtx->builder.CreateCondBr(irCtx->builder.CreateICmpEQ(firstTag, secondTag), sameTagTrueBlock->get_bb(),
		                            sameTagFalseBlock->get_bb());
		sameTagTrueBlock->set_active(irCtx->builder);
		ir::Block* cmpTrueBlock  = nullptr;
		ir::Block* cmpFalseBlock = nullptr;
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_copy_assignable()) {
				auto* subTy   = subtypes.at(i).second.value();
				cmpTrueBlock  = ir::Block::create(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
				cmpFalseBlock = ir::Block::create(fun, cmpFalseBlock ? cmpFalseBlock : sameTagTrueBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        firstTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    cmpTrueBlock->get_bb(), cmpFalseBlock->get_bb());
				cmpTrueBlock->set_active(irCtx->builder);
				subTy->move_assign_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            firstData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                              irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            secondData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                               irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(false, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				cmpFalseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		sameTagFalseBlock->set_active(irCtx->builder);
		ir::Block* firstCmpTrueBlock  = nullptr;
		ir::Block* firstCmpFalseBlock = nullptr;
		ir::Block* firstCmpRestBlock  = ir::Block::create(fun, sameTagFalseBlock);
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_destructible()) {
				auto* subTy       = subtypes.at(i).second.value();
				firstCmpTrueBlock = ir::Block::create(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
				firstCmpFalseBlock =
				    ir::Block::create(fun, firstCmpFalseBlock ? firstCmpFalseBlock : sameTagFalseBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        firstTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    firstCmpTrueBlock->get_bb(), firstCmpFalseBlock->get_bb());
				firstCmpTrueBlock->set_active(irCtx->builder);
				subTy->destroy_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            firstData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                              irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, firstCmpRestBlock->get_bb());
				firstCmpFalseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, firstCmpRestBlock->get_bb());
		firstCmpRestBlock->set_active(irCtx->builder);
		ir::Block* secondCmpTrueBlock  = nullptr;
		ir::Block* secondCmpFalseBlock = nullptr;
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_copy_constructible()) {
				auto* subTy = subtypes.at(i).second.value();
				secondCmpTrueBlock =
				    ir::Block::create(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
				secondCmpFalseBlock =
				    ir::Block::create(fun, secondCmpFalseBlock ? secondCmpFalseBlock : firstCmpRestBlock);
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        secondTag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    secondCmpTrueBlock->get_bb(), secondCmpFalseBlock->get_bb());
				secondCmpTrueBlock->set_active(irCtx->builder);
				subTy->move_construct_value(
				    irCtx,

				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            firstData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                              irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            secondData, llvm::PointerType::get(subTy->get_llvm_type(),
				                                               irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(false, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				secondCmpFalseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		restBlock->set_active(irCtx->builder);
	} else {
		irCtx->Error("Could not move assign an instance of type " + irCtx->color(to_string()), None);
	}
}

bool MixType::is_destructible() const {
	for (auto sub : subtypes) {
		if (sub.second.has_value() && !sub.second.value()->is_destructible()) {
			return false;
		}
	}
	return true;
}

void MixType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (is_destructible()) {
		auto* tag =
		    irCtx->builder.CreateLoad(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth),
		                              irCtx->builder.CreateStructGEP(get_llvm_type(), instance->get_llvm(), 0u));
		auto*      dataPtr    = irCtx->builder.CreateStructGEP(get_llvm_type(), instance->get_llvm(), 1u);
		ir::Block* trueBlock  = nullptr;
		ir::Block* falseBlock = nullptr;
		ir::Block* restBlock  = ir::Block::create(fun, fun->get_block()->get_parent());
		restBlock->link_previous_block(fun->get_block());
		for (usize i = 0; i < subtypes.size(); i++) {
			if (subtypes.at(i).second.has_value() && subtypes.at(i).second.value()->is_destructible()) {
				auto* subTy = subtypes.at(i).second.value();
				trueBlock   = ir::Block::create(fun, falseBlock ? falseBlock : fun->get_block());
				falseBlock  = ir::Block::create(fun, falseBlock ? falseBlock : fun->get_block());
				irCtx->builder.CreateCondBr(
				    irCtx->builder.CreateICmpEQ(
				        tag, llvm::ConstantInt::get(llvm::Type::getIntNTy(irCtx->llctx, tagBitWidth), i, false)),
				    trueBlock->get_bb(), falseBlock->get_bb());
				trueBlock->set_active(irCtx->builder);
				subTy->destroy_value(
				    irCtx,
				    ir::Value::get(
				        irCtx->builder.CreatePointerCast(
				            dataPtr, llvm::PointerType::get(subTy->get_llvm_type(),
				                                            irCtx->dataLayout.value().getProgramAddressSpace())),
				        ir::ReferenceType::get(true, subTy, irCtx), false),
				    fun);
				(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
				falseBlock->set_active(irCtx->builder);
			}
		}
		(void)ir::add_branch(irCtx->builder, restBlock->get_bb());
		restBlock->set_active(irCtx->builder);
	} else {
		irCtx->Error("Could not destroy an instance of type " + irCtx->color(to_string()), None);
	}
}

String MixType::to_string() const { return get_full_name(); }

TypeKind MixType::type_kind() const { return TypeKind::mixType; }

} // namespace qat::ir
