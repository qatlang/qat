#include "./global_declaration.hpp"

#include <llvm/IR/Constant.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/GlobalValue.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Value.h>

namespace qat::ast {

void GlobalDeclaration::create_entity(ir::Mod* mod, ir::Ctx* irCtx) {
	SHOW("CreateEntity: " << name.value)
	mod->entity_name_check(irCtx, name, ir::EntityType::global);
	entityState = mod->add_entity(name, ir::EntityType::global, this, ir::EmitPhase::phase_1);
}

void GlobalDeclaration::update_entity_dependencies(ir::Mod* parent, ir::Ctx* irCtx) {
	auto ctx = EmitCtx::get(irCtx, parent);
	type->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	if (value.has_value()) {
		value.value()->update_dependencies(ir::EmitPhase::phase_1, ir::DependType::complete, entityState, ctx);
	}
}

void GlobalDeclaration::do_phase(ir::EmitPhase phase, ir::Mod* parent, ir::Ctx* irCtx) { define(parent, irCtx); }

void GlobalDeclaration::define(ir::Mod* mod, ir::Ctx* irCtx) {
	auto emitCtx = EmitCtx::get(irCtx, mod);
	emitCtx->name_check_in_module(name, "global entity", None);
	auto visibInfo = emitCtx->get_visibility_info(visibSpec);
	if (not type) {
		irCtx->Error("Expected a type for global declaration", fileRange);
	}
	SHOW("Emitting type")
	auto* typ = type->emit(emitCtx);
	SHOW("Got type for global")
	llvm::GlobalVariable*  gvar = nullptr;
	Maybe<llvm::Constant*> initialValue;
	Maybe<String>          foreignID;
	Maybe<String>          linkAlias;
	Maybe<ir::MetaInfo>    irMetaInfo;
	if (metaInfo.has_value()) {
		irMetaInfo = metaInfo.value().toIR(emitCtx);
		foreignID  = irMetaInfo.value().get_value_as_string_for("foreign");
		linkAlias  = irMetaInfo.value().get_value_as_string_for(ir::MetaInfo::linkAsKey);
	}
	if (not foreignID.has_value()) {
		foreignID = mod->get_relevant_foreign_id();
	}
	auto linkNames = mod->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::global), foreignID);
	linkNames.setLinkAlias(linkAlias);
	auto linkingName = linkNames.toName();
	SHOW("Global is a variable: " << (is_variable ? "true" : "false"))
	if (value.has_value()) {
		SHOW("Has value for global")
		// FIXME - Also do destruction logic
		auto* init = mod->get_mod_initialiser(irCtx);
		init->get_block()->set_active(irCtx->builder);
		auto valEmitCtx = EmitCtx::get(irCtx, mod)->with_function(init);
		SHOW("Set mod initialiser active")
		if (value.value()->isInPlaceCreatable()) {
			SHOW("Is in place creatable")
			gvar = new llvm::GlobalVariable(
			    *mod->get_llvm_module(), typ->get_llvm_type(), false, llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
			    typ->get_llvm_type()->isPointerTy()
			        ? llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(typ->get_llvm_type()))
			        : llvm::Constant::getNullValue(typ->get_llvm_type()),
			    linkingName);
			value.value()->asInPlaceCreatable()->setCreateIn(ir::Value::get(gvar, typ, false));
			SHOW("Emitting in-place creatable")
			(void)value.value()->emit(valEmitCtx);
			SHOW("Emitted in-place creatable")
			value.value()->asInPlaceCreatable()->unsetCreateIn();
		} else {
			SHOW("Emitting value")
			SHOW("Mod is " << mod)
			SHOW("CTX has fn " << valEmitCtx->has_fn())
			SHOW("CTX has mem " << valEmitCtx->has_member_parent())
			auto val = value.value()->emit(valEmitCtx);
			SHOW("Emitted value")
			if (val->is_prerun_value()) {
				SHOW("Value is prerun")
				gvar = std::construct_at(OwnNormal(llvm::GlobalVariable), *mod->get_llvm_module(), typ->get_llvm_type(),
				                         not is_variable, irCtx->getGlobalLinkageForVisibility(visibInfo),
				                         llvm::dyn_cast<llvm::Constant>(val->get_llvm()), linkingName);
				initialValue = val->get_llvm_constant();
			} else {
				SHOW("Value is not prerun")
				if (typ->is_ref()) {
					typ = typ->as_ref()->get_subtype();
				}
				mod->add_non_const_global_counter();
				gvar = std::construct_at(
				    OwnNormal(llvm::GlobalVariable), *mod->get_llvm_module(), typ->get_llvm_type(), false,
				    llvm::GlobalValue::LinkageTypes::WeakAnyLinkage,
				    typ->get_llvm_type()->isPointerTy()
				        ? llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(typ->get_llvm_type()))
				        : llvm::Constant::getNullValue(typ->get_llvm_type()),
				    linkingName);
				if (val->is_value()) {
					irCtx->builder.CreateStore(val->get_llvm(), gvar);
				} else {
					if (typ->is_trivially_copyable() || typ->is_trivially_movable()) {
						if (val->is_ref()) {
							val->load_ghost_ref(irCtx->builder);
						}
						auto origVal = val;
						auto result  = irCtx->builder.CreateLoad(typ->get_llvm_type(), val->get_llvm());
						if (not typ->is_trivially_copyable()) {
							if (origVal->is_ref() ? origVal->get_ir_type()->as_ref()->has_variability()
							                      : origVal->is_variable()) {
								irCtx->Error(
								    "This expression does not have variability and hence cannot be trivially moved from",
								    value.value()->fileRange);
							}
							irCtx->builder.CreateStore(llvm::Constant::getNullValue(typ->get_llvm_type()),
							                           origVal->get_llvm());
						}
						irCtx->builder.CreateStore(result, gvar);
					} else {
						irCtx->Error("This expression is a reference to the type " + irCtx->color(typ->to_string()) +
						                 " which is not trivially copyable or movable. Please use " +
						                 irCtx->color("'copy") + " or " + irCtx->color("'move") + " accordingly",
						             fileRange);
					}
				}
			}
		}
	} else {
		if (not foreignID.has_value()) {
			irCtx->Error(
			    "This global entity is not a foreign entity, and not part of a foreign module, so it is required to have a value",
			    fileRange);
		}
		SHOW("Creating global variable")
		gvar = std::construct_at(OwnNormal(llvm::GlobalVariable), *mod->get_llvm_module(), typ->get_llvm_type(),
		                         not is_variable, llvm::GlobalValue::LinkageTypes::ExternalWeakLinkage, nullptr,
		                         linkingName, nullptr, llvm::GlobalValue::ThreadLocalMode::NotThreadLocal, None, true);
		SHOW("Created global")
	}
	(void)ir::GlobalEntity::create(mod, name, typ, is_variable, initialValue, gvar, visibInfo);
	SHOW("Created global entity")
}

Json GlobalDeclaration::to_json() const {
	return Json()
	    ._("nodeType", "globalDeclaration")
	    ._("name", name)
	    ._("type", type ? type->to_json() : Json())
	    ._("hasValue", value.has_value())
	    ._("value", value ? value.value()->to_json() : JsonValue())
	    ._("variability", is_variable)
	    ._("hasVisibility", visibSpec.has_value())
	    ._("visibility", visibSpec.has_value() ? visibSpec.value().to_json() : JsonValue())
	    ._("fileRange", fileRange);
}

} // namespace qat::ast
