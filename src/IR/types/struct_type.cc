#include "./struct_type.hpp"
#include "../../ast/define_struct_type.hpp"
#include "../../ast/types/generic_abstract.hpp"
#include "../../show.hpp"
#include "../generics.hpp"
#include "../logic.hpp"
#include "../qat_module.hpp"
#include "./expanded_type.hpp"
#include "./qat_type.hpp"
#include "reference.hpp"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <utility>

namespace qat::ir {

StructType::StructType(Mod* mod, Identifier _name, Vec<GenericArgument*> _generics, ir::OpaqueType* _opaqued,
                       Vec<StructField*> _members, const VisibilityInfo& _visibility, llvm::LLVMContext& llctx,
                       Maybe<MetaInfo> _metaInfo, bool isPacked)
    : ExpandedType(std::move(_name), _generics, mod, _visibility), EntityOverview("structType", Json(), _name.range),
      opaquedType(_opaqued), members(std::move(_members)), metaInfo(_metaInfo) {
	SHOW("Generating LLVM Type for struct type members")
	Vec<llvm::Type*> subtypes;
	for (auto* mem : members) {
		subtypes.push_back(mem->type->get_llvm_type());
	}
	metaInfo = opaquedType->metaInfo;
	SHOW("All members' LLVM types obtained")
	llvmType    = opaquedType->get_llvm_type();
	linkingName = opaquedType->get_name_for_linking();
	llvm::cast<llvm::StructType>(llvmType)->setBody(subtypes, isPacked);
	if (not is_generic()) {
		mod->structTypes.push_back(this);
	}
	opaquedType->set_sub_type(this);
	ovInfo            = opaquedType->ovInfo;
	ovMentions        = opaquedType->ovMentions;
	ovBroughtMentions = opaquedType->ovBroughtMentions;
	ovRange           = opaquedType->ovRange;
}

LinkNames StructType::get_link_names() const {
	Maybe<String> foreignID;
	Maybe<String> linkAlias;
	if (metaInfo) {
		foreignID = metaInfo->get_foreign_id();
		linkAlias = metaInfo->get_value_as_string_for(ir::MetaInfo::linkAsKey);
	}
	if (not foreignID.has_value()) {
		foreignID = parent->get_relevant_foreign_id();
	}
	auto linkNames = parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::type), foreignID);
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

StructType::~StructType() {
	for (auto* mem : members) {
		std::destroy_at(mem);
	}
	for (auto* gen : generics) {
		std::destroy_at(gen);
	}
}

void StructType::update_overview() {
	Vec<JsonValue> memJson;
	for (auto* mem : members) {
		memJson.push_back(mem->overviewToJson());
	}
	Vec<JsonValue> statMemJson;
	for (auto* statMem : staticMembers) {
		statMemJson.push_back(statMem->overviewToJson());
	}
	Vec<JsonValue> memFnJson;
	for (auto* mFn : memberFunctions) {
		memFnJson.push_back(mFn->overviewToJson());
	}
	Vec<JsonValue> genericArgumentsJSON;
	for (auto* arg : generics) {
		genericArgumentsJSON.push_back(arg->to_json());
	}
	ovInfo = Json()
	             ._("typeID", get_id())
	             ._("fullName", get_full_name())
	             ._("genericArguments", genericArgumentsJSON)
	             ._("moduleID", parent->get_id())
	             ._("members", memJson)
	             ._("staticFields", statMemJson)
	             ._("memberFunctions", memFnJson)
	             ._("isDefaultConstructible", is_default_constructible())
	             ._("isDestructible", is_destructible())
	             ._("hasSimpleCopy", has_simple_copy())
	             ._("hasSimpleMove", has_simple_move())
	             ._("isCopyConstructible", is_copy_constructible())
	             ._("isMoveConstructible", is_move_constructible())
	             ._("isCopyAssignable", is_copy_assignable())
	             ._("isMoveAssignable", is_move_assignable())
	             ._("visibility", visibility);
}

u64 StructType::get_field_count() const { return members.size(); }

Vec<StructField*>& StructType::get_members() { return members; }

bool StructType::has_field_with_name(const String& member) const {
	for (auto* mem : members) {
		if (mem->name.value == member) {
			return true;
		}
	}
	return false;
}

StructField* StructType::get_field_with_name(const String& member) const {
	for (auto* mem : members) {
		if (mem->name.value == member) {
			return mem;
		}
	}
	return nullptr;
}

StructField* StructType::get_field_at(u64 index) { return members.at(index); }

Maybe<usize> StructType::get_index_of(const String& member) const {
	Maybe<usize> result;
	for (usize i = 0; i < members.size(); i++) {
		if (members.at(i)->name.value == member) {
			result = i;
			break;
		}
	}
	return result;
}

String StructType::get_field_name_at(u64 index) const {
	return (index < members.size()) ? members.at(index)->name.value : "";
}

usize StructType::get_field_index(String const& memName) const {
	for (usize i = 0; i < members.size(); i++) {
		if (members[i]->name.value == memName) {
			return i;
		}
	}
}

Type* StructType::get_type_of_field(const String& member) const {
	Maybe<usize> result;
	for (usize i = 0; i < members.size(); i++) {
		if (members.at(i)->name.value == member) {
			result = i;
			break;
		}
	}
	if (result) {
		return members.at(result.value())->type;
	} else {
		return nullptr;
	}
}

bool StructType::has_static_field(const String& _name) const {
	for (auto* stm : staticMembers) {
		if (stm->get_name().value == _name) {
			return true;
		}
	}
	return false;
}

StaticMember* StructType::get_static_field(String const& name) const {
	for (auto* stm : staticMembers) {
		if (stm->get_name().value == name) {
			return stm;
		}
	}
	return nullptr;
}

bool StructType::is_type_sized() const { return not members.empty(); }

bool StructType::has_simple_copy() const {
	if (explicitSimpleCopy) {
		return true;
	} else if (has_copy_constructor() || has_copy_assignment()) {
		return false;
	} else {
		auto result = true;
		for (auto mem : members) {
			if (not mem->type->has_simple_copy()) {
				result = false;
				break;
			}
		}
		return result;
	}
}

bool StructType::has_simple_move() const {
	if (explicitSimpleMove) {
		return true;
	} else if (has_move_constructor() || has_move_assignment()) {
		return false;
	} else {
		for (auto mem : members) {
			if (not mem->type->has_simple_move()) {
				return false;
			}
		}
		return true;
	}
}

bool StructType::is_copy_constructible() const {
	if (has_simple_copy() || has_copy_constructor()) {
		return true;
	} else {
		bool allMemsRes = true;
		for (auto* mem : members) {
			if (not mem->type->is_copy_constructible()) {
				allMemsRes = false;
				break;
			}
		}
		return allMemsRes;
	}
}

bool StructType::is_copy_assignable() const {
	if (has_simple_copy() || has_copy_assignment()) {
		return true;
	} else {
		bool allMemsRes = true;
		for (auto* mem : members) {
			if (not mem->type->is_copy_assignable()) {
				allMemsRes = false;
				break;
			}
		}
		return allMemsRes;
	}
}

bool StructType::is_move_constructible() const {
	if (has_simple_move() || has_move_constructor()) {
		return true;
	} else {
		bool allMemsRes = true;
		for (auto* mem : members) {
			if (not mem->type->is_move_constructible()) {
				allMemsRes = false;
				break;
			}
		}
		return allMemsRes;
	}
}

bool StructType::is_move_assignable() const {
	if (has_simple_move() || has_move_assignment()) {
		return true;
	} else {
		bool allMemsRes = true;
		for (auto* mem : members) {
			if (not mem->type->is_move_assignable()) {
				allMemsRes = false;
				break;
			}
		}
		return allMemsRes;
	}
}

bool StructType::is_destructible() const {
	if (has_simple_move() || has_destructor()) {
		return true;
	} else {
		bool allMemsRes = true;
		for (auto* mem : members) {
			if (not mem->type->is_destructible()) {
				allMemsRes = false;
				break;
			}
		}
		return allMemsRes;
	}
}

void StructType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (not is_copy_constructible()) {
		irCtx->Error("Could not copy construct an instance of type " + irCtx->color(to_string()), None);
	}
	if (has_simple_copy()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
	} else if (has_copy_constructor()) {
		(void)get_copy_constructor()->call(irCtx, {first->get_llvm(), second->get_llvm()}, None, fun->get_module());
	} else {
		for (usize i = 0; i < members.size(); i++) {
			members.at(i)->type->copy_construct_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), i),
			                   ir::RefType::get(true, members.at(i)->type, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), i),
			                   ir::RefType::get(false, members.at(i)->type, irCtx), false),
			    fun);
		}
	}
}

void StructType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (not is_copy_assignable()) {
		irCtx->Error("Could not copy assign an instance of type " + irCtx->color(to_string()), None);
	}
	if (has_simple_copy()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
	} else if (has_copy_assignment()) {
		(void)get_copy_assignment()->call(irCtx, {first->get_llvm(), second->get_llvm()}, None, fun->get_module());
	} else {
		for (usize i = 0; i < members.size(); i++) {
			members.at(i)->type->copy_assign_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), i),
			                   ir::RefType::get(true, members.at(i)->type, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), i),
			                   ir::RefType::get(false, members.at(i)->type, irCtx), false),
			    fun);
		}
	}
}

void StructType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (not is_move_constructible()) {
		irCtx->Error("Could not move construct an instance of type " + irCtx->color(to_string()), None);
	}
	if (has_simple_move()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), second->get_llvm());
	} else if (has_move_constructor()) {
		(void)get_move_constructor()->call(irCtx, {first->get_llvm(), second->get_llvm()}, None, fun->get_module());
	} else {
		for (usize i = 0; i < members.size(); i++) {
			members.at(i)->type->move_construct_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), i),
			                   ir::RefType::get(true, members.at(i)->type, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), i),
			                   ir::RefType::get(false, members.at(i)->type, irCtx), false),
			    fun);
		}
	}
}

void StructType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (not is_move_assignable()) {
		irCtx->Error("Could not move assign an instance of type " + irCtx->color(to_string()), None);
	}
	if (has_simple_move()) {
		irCtx->builder.CreateStore(irCtx->builder.CreateLoad(get_llvm_type(), second->get_llvm()), first->get_llvm());
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), second->get_llvm());
	} else if (has_move_assignment()) {
		(void)get_move_assignment()->call(irCtx, {first->get_llvm(), second->get_llvm()}, None, fun->get_module());
	} else {
		for (usize i = 0; i < members.size(); i++) {
			members.at(i)->type->move_assign_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), first->get_llvm(), i),
			                   ir::RefType::get(true, members.at(i)->type, irCtx), false),
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), second->get_llvm(), i),
			                   ir::RefType::get(false, members.at(i)->type, irCtx), false),
			    fun);
		}
	}
}

void StructType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (not is_destructible()) {
		irCtx->Error("Could not destroy an instance of type " + irCtx->color(to_string()), None);
	}
	if (has_destructor()) {
		(void)get_destructor()->call(irCtx, {instance->get_llvm()}, None, fun->get_module());
	} else if (has_simple_move()) {
		irCtx->builder.CreateStore(llvm::Constant::getNullValue(get_llvm_type()), instance->get_llvm());
	} else {
		for (usize i = 0; i < members.size(); i++) {
			members.at(i)->type->destroy_value(
			    irCtx,
			    ir::Value::get(irCtx->builder.CreateStructGEP(get_llvm_type(), instance->get_llvm(), i),
			                   ir::RefType::get(true, members.at(i)->type, irCtx), false),
			    fun);
		}
	}
}

void StructType::add_static_member(Identifier const& name, Type* type, bool variability, Value* initial,
                                   VisibilityInfo const& visibility, llvm::LLVMContext& llctx) {
	staticMembers.push_back(StaticMember::get(this, name, type, variability, initial, visibility));
}

TypeKind StructType::type_kind() const { return TypeKind::STRUCT; }

String StructType::to_string() const { return get_full_name(); }

GenericStructType::GenericStructType(Identifier _name, Vec<ast::GenericAbstractType*> _generics,
                                     ast::PrerunExpression* _constraint, ast::DefineStructType* _defineStructType,
                                     Mod* _parent, const VisibilityInfo& _visibInfo)
    : EntityOverview("genericStructType",
                     Json()
                         ._("name", _name.value)
                         ._("fullName", _parent->get_fullname_with_child(_name.value))
                         ._("visibility", _visibInfo)
                         ._("moduleID", _parent->get_id())
                         ._("visibility", _visibInfo),
                     _name.range),
      name(std::move(_name)), generics(_generics), defineStructType(_defineStructType), parent(_parent),
      visibility(_visibInfo), constraint(_constraint) {
	parent->genericStructTypes.push_back(this);
}

void GenericStructType::update_overview() {
	Vec<JsonValue> genericParamsJson;
	for (auto* param : generics) {
		genericParamsJson.push_back(param->to_json());
	}
	Vec<JsonValue> variantsJson;
	for (auto var : variants) {
		variantsJson.push_back(var.get()->overviewToJson());
	}
	ovInfo._("genericParameters", genericParamsJson)
	    ._("hasConstraint", constraint != nullptr)
	    ._("constraint", constraint ? constraint->to_string() : JsonValue())
	    ._("variants", variantsJson);
}

Identifier GenericStructType::get_name() const { return name; }

VisibilityInfo GenericStructType::get_visibility() const { return visibility; }

bool GenericStructType::all_parameters_have_default() const {
	for (auto* gen : generics) {
		if (not gen->hasDefault()) {
			return false;
		}
	}
	return true;
}

usize GenericStructType::get_parameter_count() const { return generics.size(); }

usize GenericStructType::getVariantCount() const { return variants.size(); }

Mod* GenericStructType::get_module() const { return parent; }

ast::GenericAbstractType* GenericStructType::getGenericAt(usize index) const { return generics.at(index); }

Type* GenericStructType::fill_generics(Vec<GenericToFill*>& toFillTypes, ir::Ctx* irCtx, FileRange range) {
	for (auto& oVar : opaqueVariants) {
		SHOW("Opaque variant: " << oVar.get()->get_full_name())
		if (oVar.check(irCtx, [&](const String& msg, const FileRange& rng) { irCtx->Error(msg, rng); }, toFillTypes)) {
			return oVar.get();
		}
	}
	for (auto& var : variants) {
		SHOW("Struct type variant: " << var.get()->get_full_name())
		if (var.check(irCtx, [&](const String& msg, const FileRange& rng) { irCtx->Error(msg, rng); }, toFillTypes)) {
			return var.get();
		}
	}
	auto* ctx = ast::EmitCtx::get(irCtx, parent);
	ir::fill_generics(ctx, generics, toFillTypes, range);
	if (constraint != nullptr) {
		auto checkVal = constraint->emit(ctx);
		if (not checkVal->get_ir_type()->is_bool()) {
			irCtx->Error("The constraints for generic parameters should be of " + irCtx->color("bool") +
			                 " type. Got an expression of " + irCtx->color(checkVal->get_ir_type()->to_string()),
			             constraint->fileRange);
		}
		if (not llvm::cast<llvm::ConstantInt>(checkVal->get_llvm_constant())->getValue().getBoolValue()) {
			irCtx->Error("The provided parameters for the generic struct type do not satisfy the constraints", range,
			             Pair<String, FileRange>{"The constraint can be found here", constraint->fileRange});
		}
	}
	Vec<ir::GenericArgument*> genParams;
	for (auto genAb : generics) {
		genParams.push_back(genAb->toIRGenericType());
	}
	auto variantName = ir::Logic::get_generic_variant_name(name.value, toFillTypes);
	if (variantNames.contains(variantName)) {
		irCtx->Error("Repeating variant name: " + variantName, range);
	}
	variantNames.insert(variantName);
	irCtx->add_active_generic(
	    ir::GenericEntityMarker{
	        variantName,
	        ir::GenericEntityType::structType,
	        range,
	        0u,
	        genParams,
	    },
	    true);
	auto* resultTy = defineStructType->create_type(toFillTypes, parent, irCtx);
	defineStructType->create_type_definitions(resultTy, parent, irCtx);
	defineStructType->do_define(resultTy, parent, irCtx);
	defineStructType->do_emit(resultTy, irCtx);
	for (auto* temp : generics) {
		temp->unset();
	}
	if (irCtx->get_active_generic().warningCount > 0) {
		auto count = irCtx->get_active_generic().warningCount;
		irCtx->Warning(std::to_string(count) + " warning" + (count > 1 ? "s" : "") +
		                   " generated while creating generic variant " + irCtx->highlightWarning(variantName),
		               range);
		irCtx->remove_active_generic();
	} else {
		irCtx->remove_active_generic();
	}
	SHOW("Returning struct type")
	return resultTy;
}

} // namespace qat::ir
