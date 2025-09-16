#include "./get_poly.hpp"
#include "../../IR/type_id.hpp"
#include "../../IR/types/polymorph.hpp"

namespace qat::ast {

ir::Value* GetPolymorph::emit(EmitCtx* ctx) {
	auto            val = value->emit(ctx);
	Vec<ir::Skill*> skillsIR;
	if (val->get_ir_type()->is_poly() ||
	    (val->get_ir_type()->is_ref() && val->get_ir_type()->as_ref()->get_subtype()->is_poly())) {
		for (auto& sk : skills) {
			if (sk.is_done_skill()) {
				ctx->Error(
				    "Providing skill implementations to be used, is not supported while extracting a polymorph from another polymorph",
				    sk.range);
			}
			skillsIR.push_back(sk.as_skill().find_skill(ctx));
		}
		Maybe<ir::AddressSpace> addr;
		if (addressSpace.has_value()) {
			addr = addressSpace.value().to_ir(ctx);
		}
		auto resTy = ir::Polymorph::create(
		    isTypeRange.has_value(), isVar, std::move(skillsIR),
		    owner.has_value() ? Maybe<ir::PtrOwner>(get_ptr_owner(ctx, owner.value(), fileRange)) : None,
		    std::move(addr), ctx->irCtx);

		if (val->get_ir_type()->is_ref()) {
			val->load_ghost_ref(ctx->irCtx->builder);
			val = ir::Value::get(ctx->irCtx->builder.CreateLoad(
			                         val->get_ir_type()->as_ref()->get_subtype()->get_llvm_type(), val->get_llvm()),
			                     val->get_ir_type()->as_ref()->get_subtype(), true);
		} else {
			val->load_ghost_ref(ctx->irCtx->builder);
		}
		auto origTy = val->get_ir_type()->as_poly();
		if (resTy->has_owner() != origTy->has_owner()) {
			if (not resTy->has_owner() && not origTy->get_owner().is_none()) {
				ctx->Error(
				    "The existing polymorph here of type " + ctx->color(origTy->to_string()) +
				        " is a pointer polymorph, but the resultant polymorph of type " +
				        ctx->color(resTy->to_string()) +
				        " is a reference polymorph. Reference polymorphs can be extracted only from a pointer polymorph with anonymous ownership."
				        " Please check the existing pointer polymorph to be safe, and retrieve a reference polymorph from it"
				        " if you want to do so, via pattern matching or conversion",
				    fileRange);
			} else if (not origTy->has_owner() && not resTy->get_owner().is_none()) {
				ctx->Error(
				    "The existing polymorph here of type " + ctx->color(origTy->to_string()) +
				        " is a reference polymorph, but the resultant polymorph of type " +
				        ctx->color(resTy->to_string()) +
				        " is a pointer polymorph with ownership. Only pointer polymorphs with anonymous ownership can be extracted"
				        " from a reference polymorph",
				    fileRange);
			}
		} else if ((resTy->has_owner() && origTy->has_owner()) && not resTy->get_owner().is_same(origTy->get_owner()) &&
		           not resTy->get_owner().is_none()) {
			ctx->Error(
			    "The pointer ownership of the resultant pointer polymorph of type " + ctx->color(resTy->to_string()) +
			        " is not compatible with the pointer ownership of the existing polymorph of type " +
			        ctx->color(origTy->to_string()) +
			        ". Only pointer polymorphs with anonymous ownership can be extracted from pointer polymorphs with other ownership types",
			    fileRange);
		} else if (resTy->has_address_space() != origTy->has_address_space()) {
			ctx->Error("The existing polymorph is of type " + ctx->color(origTy->to_string()) +
			               ", but the resultant polymorph is of type " + ctx->color(resTy->to_string()) +
			               ". The address-space specification does not match",
			           fileRange);
		} else if (resTy->has_address_space() && origTy->has_address_space() &&
		           not resTy->get_address_space().value().is_same(origTy->get_address_space().value())) {
			ctx->Error("The existing polymorph has an address-space of " +
			               ctx->color(origTy->get_address_space().value().to_string()) +
			               ", but the resultant polymorph has an address-space of " +
			               ctx->color(resTy->get_address_space().value().to_string()) + ". These do not match",
			           fileRange);
		}
		if (resTy->is_typed_poly() && not origTy->is_typed_poly()) {
			ctx->Error("The existing polymorph here of type " + ctx->color(origTy->to_string()) +
			               ", is not a typed polymorph, so it is not possible to extract a typed polymorph from it",
			           fileRange);
		}
		if (resTy->has_variability() && not origTy->has_variability()) {
			ctx->Error("The existing polymorph here of type " + ctx->color(origTy->to_string()) +
			               " does not have variability, so a polymorph with variability cannot be extracted from it",
			           fileRange);
		}
		auto& targSk = resTy->get_skills();
		for (auto* sk : targSk) {
			if (not origTy->has_skill(sk)) {
				ctx->Error("Cannot retrieve the polymorph " + ctx->color(resTy->to_string()) +
				               " from the existing polymorph " + ctx->color(origTy->to_string()) + " as the skill " +
				               ctx->color(sk->get_full_name()) + " is missing in the original polymorph",
				           fileRange);
			}
		}
		auto loc =
		    ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), resTy, false, fileRange);
		if (origTy->has_owner() && not origTy->get_owner().is_none() &&
		    llvm::cast<llvm::StructType>(origTy->get_llvm_type())->getElementType(0)->isStructTy() &&
		    (not resTy->has_owner() || resTy->get_owner().is_none())) {
			ctx->irCtx->builder.CreateStore(
			    ctx->irCtx->builder.CreateExtractValue(ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}),
			                                           {0u}),
			    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(), 0u));
		} else {
			ctx->irCtx->builder.CreateStore(
			    ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}),
			    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(), 0u));
		}
		if (resTy->is_typed_poly()) {
			ctx->irCtx->builder.CreateStore(
			    ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {1u}),
			    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(), 1u));
		}
		for (auto sk : resTy->get_skills()) {
			ctx->irCtx->builder.CreateStore(
			    ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {(uint)origTy->get_skill_index_in_type(sk)}),
			    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(),
			                                        resTy->get_skill_index_in_type(sk)));
		}
		return ir::Value::get(ctx->irCtx->builder.CreateLoad(resTy->get_llvm_type(), loc->get_llvm()), resTy, false);
	} else {
		if (not val->is_ref() && not val->is_ptr() && not val->is_ghost_ref()) {
			ctx->Error(
			    "Expected a reference-like or a pointer expression here. Polymorphs cannot be retrieved for values, please use " +
			        ctx->color("'let") +
			        " to allocate this value in-place and convert this to a reference-like expression or store this value in a local declaration first",
			    value->fileRange);
		} else if (val->is_ptr()) {
			if (val->get_ir_type()->as_ptr()->is_multi()) {
				ctx->Error(
				    "Polymorphs cannot be retrieved from multi-pointers, instead they can only be retrieved for each element in the multi-pointer, separately",
				    value->fileRange);
			} else if (not val->get_ir_type()->as_ptr()->is_non_nullable()) {
				ctx->Error(
				    "Polymorphs cannot be retrieved from nullable pointers. Please check if this is a null pointer and convert it into a non-nullable pointer."
				    " If you are absolutely sure that this pointer cannot be null, use ! at the end to affirm this to be a non-nullable pointer",
				    value->fileRange);
			}
		}
		if (val->is_ref() || val->is_ptr()) {
			val->load_ghost_ref(ctx->irCtx->builder);
		}
		auto            targetType = val->is_ptr()
		                                 ? val->get_ir_type()->as_ptr()->get_subtype()
		                                 : (val->is_ref() ? val->get_ir_type()->as_ref()->get_subtype() : val->get_ir_type());
		Vec<ir::Skill*> skillsIR{};
		skills.reserve(skills.size());
		Vec<ir::DoneSkill*> doneSkills{};
		doneSkills.reserve(skills.size());
		for (auto& sk : skills) {
			if (sk.is_skill()) {
				auto skRes = sk.as_skill().find_skill(ctx);
				if (not skRes->can_be_polymorph()) {
					ctx->Error("The skill " + ctx->color(skRes->get_full_name()) +
					               " cannot be a polymorph, as it has been marked to disable polymorphs",
					           sk.range);
				}
				if (not targetType->has_unnamed_implementation_for(skRes)) {
					String extraInfo;
					bool   moreThanOne = false;
					if (targetType->has_named_implementation_for(skRes)) {
						auto allDoneSkills = targetType->get_named_implementations_for(skRes);
						moreThanOne        = allDoneSkills.size() > 1;
						for (usize i = 0; i < allDoneSkills.size(); i++) {
							extraInfo += allDoneSkills[i]->get_full_name() + " in " +
							             allDoneSkills[i]->get_module()->get_referrable_name();
							if (i != (allDoneSkills.size())) {
								extraInfo += "\n";
							}
						}
					}
					ctx->Error(
					    "The type " + ctx->color(targetType->to_string()) +
					        " for which the polymorph has to be retrieved does not have an unnamed implementations for the skill " +
					        ctx->color(skRes->get_full_name()) +
					        (extraInfo.empty()
					             ? "."
					             : (String(". Found the following named implementation") + (moreThanOne ? "s " : " ") +
					                "of the skill " + ctx->color(skRes->get_full_name()) +
					                " for the type. Use the syntax " + ctx->color("from ImplementationName") +
					                " here to explicitly provide the implementation to create the polymorph from\n" +
					                extraInfo)),
					    sk.range);
				}
				skillsIR.push_back(skRes);
				doneSkills.push_back(targetType->get_unnamed_implementation_for(skRes));
			} else {
				auto doneRes = sk.as_done_skill().find_done_skill(ctx);
				if (not doneRes->get_candidate_type()->is_same(targetType)) {
					ctx->Error("The skill implementation " + ctx->color(sk.as_done_skill().to_string()) +
					               " is not targetting the type " + ctx->color(targetType->to_string()) +
					               ", so it cannot be used to retrieve a polymorph for the type",
					           sk.range);
				}
				if (not doneRes->get_skill()->can_be_polymorph()) {
					ctx->Error("The skill " + ctx->color(doneRes->get_skill()->get_full_name()) +
					               " of the implementation " + ctx->color(sk.as_done_skill().to_string()) +
					               " cannot be a polymorph as it has been marked to disable polymorphs",
					           sk.range);
				}
				doneSkills.push_back(doneRes);
				skillsIR.push_back(doneSkills.back()->get_skill());
			}
		}
		Maybe<ir::PtrOwner> ptrOwner;
		if (owner.has_value()) {
			ptrOwner = get_ptr_owner(ctx, owner.value(), owner.value().range);
		}
		Maybe<ir::AddressSpace> addr;
		if (addressSpace.has_value()) {
			addr = addressSpace.value().to_ir(ctx);
		}
		auto resTy             = ir::Polymorph::create(isTypeRange.has_value(), isVar, skillsIR, std::move(ptrOwner),
		                                               std::move(addr), ctx->irCtx);
		bool storeInstanceAsIs = true;
		if (not ptrOwner.has_value()) {
			if (val->is_ptr() && not val->get_ir_type()->as_ptr()->get_owner().is_none()) {
				ctx->Error(
				    "Trying to get a reference polymorph of " + ctx->color(resTy->to_string()) +
				        " from an expression which is a pointer with ownership. Reference polymorphs can be extracted only from reference-like expressions or from pointers with anonymous ownership",
				    fileRange);
			}
		} else {
			if (not ptrOwner.value().is_none() && val->is_ptr() &&
			    not ptrOwner.value().is_same(val->get_ir_type()->as_ptr()->get_owner())) {
				ctx->Error(
				    "Trying to get a pointer polymorph of " + ctx->color(resTy->to_string()) +
				        " from a pointer expression of type " + ctx->color(val->get_ir_type()->to_string()) +
				        ". The ownership of the pointer polymorph and that of the pointer expression does not match",
				    fileRange);
			}
			if (ptrOwner.value().is_none() && val->is_ptr() &&
			    llvm::isa<llvm::StructType>(val->get_ir_type()->get_llvm_type())) {
				storeInstanceAsIs = false;
			}
		}
		auto loc =
		    ctx->get_fn()->get_block()->new_local(ctx->get_fn()->get_random_alloca_name(), resTy, false, fileRange);
		if (storeInstanceAsIs) {
			ctx->irCtx->builder.CreateStore(
			    val->get_llvm(), ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(), 0u));
		} else {
			ctx->irCtx->builder.CreateStore(
			    ctx->irCtx->builder.CreateExtractValue(val->get_llvm(), {0u}),
			    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(), 0u));
		}
		if (isTypeRange.has_value()) {
			ctx->irCtx->builder.CreateStore(
			    ir::TypeInfo::create(ctx->irCtx, targetType, ctx->mod)->id,
			    ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(), 1u));
		}
		for (usize i = 0; i < skillsIR.size(); i++) {
			auto                  methodTable      = doneSkills[i]->get_method_table(ctx->irCtx);
			llvm::GlobalVariable* methodTableFinal = nullptr;
			if (not ctx->mod->get_llvm_module()->getGlobalVariable(methodTable->getName())) {
				methodTableFinal =
				    new llvm::GlobalVariable(*ctx->mod->get_llvm_module(), methodTable->getValueType(), true,
				                             methodTable->getLinkage(), nullptr, methodTable->getName(), nullptr,
				                             methodTable->getThreadLocalMode(), methodTable->getAddressSpace(), true);
			}
			ctx->mod->add_dependency(doneSkills[i]->get_module());
			methodTableFinal = ctx->mod->get_llvm_module()->getGlobalVariable(methodTable->getName());
			ctx->irCtx->builder.CreateStore(
			    methodTableFinal, ctx->irCtx->builder.CreateStructGEP(resTy->get_llvm_type(), loc->get_llvm(),
			                                                          resTy->get_skill_index_in_type(skillsIR[i])));
		}
		return ir::Value::get(ctx->irCtx->builder.CreateLoad(resTy->get_llvm_type(), loc->get_llvm()), resTy, false);
	}
}

} // namespace qat::ast
