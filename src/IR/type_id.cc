#include "./type_id.hpp"
#include "./context.hpp"
#include "./qat_module.hpp"
#include "./types/choice.hpp"
#include "./types/flag.hpp"
#include "./types/region.hpp"
#include "./types/struct_type.hpp"
#include "link_names.hpp"

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Type.h>

namespace qat::ir {

std::unordered_map<llvm::Constant*, TypeInfo*> TypeInfo::idMapping{};

llvm::StructType* TypeInfo::typeInfoType = nullptr;

Vec<llvm::Constant*> TypeInfo::builtinTypeInfos{};

llvm::GlobalVariable* TypeInfo::builtinGlobal = nullptr;

Mod* TypeInfo::builtinModule = nullptr;

Vec<ModTypeInfo*> TypeInfo::modules{};

TypeInfo* TypeInfo::get_for(llvm::Constant* id) { return idMapping[id]; }

TypeInfo* TypeInfo::create(ir::Ctx* ctx, Type* type, Mod* mod) {
	String correctGlobalName;
	if (type->typeInfo) {
		mod->add_dependency(type->typeInfo->mod);
		correctGlobalName =
		    type->typeInfo->mod->get_link_names().newWith(LinkNameUnit("info", LinkUnitType::global), None).toName();
	} else {
		bool isPrimitive = not(type->is_choice() || type->is_flag() || type->is_type_definition() || type->is_mix() ||
		                       type->is_struct() || type->is_region());
		llvm::Constant*       id;
		llvm::ConstantStruct* info;
		if (not typeInfoType) {
			typeInfoType =
			    llvm::StructType::create({ir::TextType::get(ctx, false)->get_llvm_type()}, "qat.typeid", false);
		}
		if (isPrimitive) {
			if (not builtinModule) {
				builtinModule =
				    Mod::create({"type", FileRange("")}, "", "", ModuleType::lib, VisibilityInfo::pub(), ctx);
				builtinGlobal = new llvm::GlobalVariable(
				    *builtinModule->get_llvm_module(), llvm::ArrayType::get(typeInfoType, UINT64_MAX), true,
				    llvm::GlobalVariable::ExternalLinkage, nullptr,
				    builtinModule->get_link_names()
				        .newWith(LinkNameUnit("info_initial", LinkUnitType::global), None)
				        .toName(),
				    nullptr, llvm::GlobalValue::NotThreadLocal, ctx->dataLayout.getDefaultGlobalsAddressSpace(), false);
			}
			info = (llvm::ConstantStruct*)llvm::ConstantStruct::get(
			    typeInfoType, {TextType::create_value(ctx, builtinModule, type->to_string())->get_llvm_constant()});
			builtinTypeInfos.push_back(info);
			id = llvm::ConstantExpr::getGetElementPtr(
			    llvm::ArrayType::get(typeInfoType, builtinTypeInfos.size()), builtinGlobal,
			    llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx), builtinTypeInfos.size() - 1, false));
			correctGlobalName =
			    builtinModule->get_link_names().newWith(LinkNameUnit("info", LinkUnitType::global), None).toName();
			mod->add_dependency(builtinModule);
			type->typeInfo = std::construct_at(OwnNormal(TypeInfo), id, info, type, builtinModule);
		} else {
			Mod* typeMod = nullptr;
			if (type->is_choice()) {
				typeMod = type->as_choice()->get_module();
			} else if (type->is_flag()) {
				typeMod = type->as_flag()->get_module();
			} else if (type->is_type_definition()) {
				typeMod = type->as_type_definition()->get_module();
			} else if (type->is_mix()) {
				typeMod = type->as_mix()->get_module();
			} else if (type->is_struct()) {
				typeMod = type->as_struct()->get_module();
			} else if (type->is_region()) {
				typeMod = type->as_region()->get_module();
			}
			if (not typeMod->typeInfoDetail) {
				auto infoMod  = Mod::create_submodule(typeMod, "", "", {"type", FileRange("")}, ModuleType::lib,
				                                      VisibilityInfo::pub(), ctx);
				auto infoList = new llvm::GlobalVariable(
				    *infoMod->get_llvm_module(), llvm::ArrayType::get(typeInfoType, UINT64_MAX), true,
				    llvm::GlobalValue::ExternalLinkage, nullptr,
				    infoMod->get_link_names()
				        .newWith(LinkNameUnit("info_initial", LinkUnitType::global), None)
				        .toName(),
				    nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal,
				    ctx->dataLayout.getDefaultGlobalsAddressSpace(), false);
				typeMod->typeInfoDetail = ModTypeInfo::create(typeMod, infoMod, infoList);
				modules.push_back(typeMod->typeInfoDetail);
			}
			info = (llvm::ConstantStruct*)llvm::ConstantStruct::get(
			    typeInfoType,
			    {TextType::create_value(ctx, typeMod->typeInfoDetail->infoMod,
			                            type->is_type_definition()
			                                ? type->as_type_definition()->get_non_definition_subtype()->to_string()
			                                : type->to_string())
			         ->get_llvm_constant()});
			typeMod->typeInfoDetail->typeInfos.push_back(info);
			id = llvm::ConstantExpr::getGetElementPtr(
			    llvm::ArrayType::get(typeInfoType, typeMod->typeInfoDetail->typeInfos.size()),
			    typeMod->typeInfoDetail->infoList,
			    llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx->llctx),
			                           typeMod->typeInfoDetail->typeInfos.size() - 1, false));
			correctGlobalName = typeMod->typeInfoDetail->infoMod->get_link_names()
			                        .newWith(LinkNameUnit("info", LinkUnitType::global), None)
			                        .toName();
			mod->add_dependency(typeMod->typeInfoDetail->infoMod);
			type->typeInfo = std::construct_at(OwnNormal(TypeInfo), id, info, type, typeMod->typeInfoDetail->infoMod);
		}
	}
	if (not mod->get_llvm_module()->getGlobalVariable(correctGlobalName)) {
		new llvm::GlobalVariable(*mod->get_llvm_module(), llvm::ArrayType::get(typeInfoType, UINT64_MAX), true,
		                         llvm::GlobalVariable::ExternalLinkage, nullptr, correctGlobalName, nullptr,
		                         llvm::GlobalValue::NotThreadLocal, ctx->dataLayout.getDefaultGlobalsAddressSpace(),
		                         true);
	}
	return type->typeInfo;
}

void TypeInfo::finalise_type_infos(ir::Ctx* ctx) {
	if (builtinModule) {
		SHOW("Finalising builtin type infos")
		auto newBuiltinGlobal = new llvm::GlobalVariable(
		    *builtinModule->get_llvm_module(), llvm::ArrayType::get(typeInfoType, builtinTypeInfos.size()), true,
		    llvm::GlobalVariable::ExternalLinkage,
		    llvm::ConstantArray::get(llvm::ArrayType::get(typeInfoType, builtinTypeInfos.size()), builtinTypeInfos),
		    builtinModule->get_link_names().newWith(LinkNameUnit("info", LinkUnitType::global), None).toName(), nullptr,
		    llvm::GlobalVariable::NotThreadLocal, ctx->dataLayout.getDefaultGlobalsAddressSpace(), false);
		builtinGlobal->replaceAllUsesWith(newBuiltinGlobal);
		builtinGlobal->eraseFromParent();
		builtinGlobal = newBuiltinGlobal;
	}
	SHOW("Finalising custom type infos")
	for (auto* mod : modules) {
		auto newGlobal = new llvm::GlobalVariable(
		    *mod->infoMod->get_llvm_module(), llvm::ArrayType::get(typeInfoType, mod->typeInfos.size()), true,
		    llvm::GlobalValue::ExternalLinkage,
		    llvm::ConstantArray::get(llvm::ArrayType::get(typeInfoType, mod->typeInfos.size()), mod->typeInfos),
		    mod->infoMod->get_link_names().newWith(LinkNameUnit("info", LinkUnitType::global), None).toName(), nullptr,
		    llvm::GlobalValue::NotThreadLocal, ctx->dataLayout.getDefaultGlobalsAddressSpace(), false);
		mod->infoList->replaceAllUsesWith(newGlobal);
		mod->infoList->eraseFromParent();
		mod->infoList = newGlobal;
	}
}

} // namespace qat::ir
