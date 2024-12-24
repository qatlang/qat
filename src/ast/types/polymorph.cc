#include "./polymorph.hpp"
#include "../../IR/skill.hpp"
#include "../../IR/types/polymorph.hpp"

namespace qat::ast {

void SkillEntity::update_dependencies(ir::EmitPhase phase, Maybe<ir::DependType> expect, ir::EntityState* ent,
                                      EmitCtx* ctx) {
	auto mod = ctx->mod;
	if (relative > 0) {
		if (not mod->has_nth_parent(relative)) {
			ctx->Error("The current module does not have " + ctx->color(std::to_string(relative)) + " parents", range);
		}
		mod = mod->get_nth_parent(relative);
	}
	auto access = ctx->get_access_info();
	for (usize i = 0; i < names.size() - 1; i++) {
		auto& name = names[i];
		if (mod->has_lib(name.value, access) || mod->has_brought_lib(name.value, access) ||
		    mod->has_lib_in_imports(name.value, access).first) {
			mod = mod->get_lib(name.value, access);
		} else if (mod->has_brought_mod(name.value, access) ||
		           mod->has_brought_mod_in_imports(name.value, access).first) {
			mod = mod->get_brought_mod(name.value, access);
		} else {
			ctx->Error("No module named " + ctx->color(name.value) + " found inside " +
			               ctx->color(mod->get_referrable_name()) + " or its submodules",
			           name.range);
		}
	}
	auto const& skName = names.back();
	if (mod->has_entity_with_name(skName.value)) {
		mod->get_entity(skName.value)
		    ->addDependency(ir::EntityDependency{ent, expect.value_or(ir::DependType::complete), phase});
	}
}

ir::Skill* SkillEntity::find(EmitCtx* ctx) const {
	auto mod = ctx->mod;
	if (relative > 0) {
		if (not mod->has_nth_parent(relative)) {
			ctx->Error("The current module does not have " + ctx->color(std::to_string(relative)) + " parents", range);
		}
		mod = mod->get_nth_parent(relative);
	}
	auto access = ctx->get_access_info();
	for (usize i = 0; i < names.size() - 1; i++) {
		auto& name = names[i];
		if (mod->has_lib(name.value, access) || mod->has_brought_lib(name.value, access) ||
		    mod->has_lib_in_imports(name.value, access).first) {
			mod = mod->get_lib(name.value, access);
		} else if (mod->has_brought_mod(name.value, access) ||
		           mod->has_brought_mod_in_imports(name.value, access).first) {
			mod = mod->get_brought_mod(name.value, access);
		} else {
			ctx->Error("No module named " + ctx->color(name.value) + " found inside " +
			               ctx->color(mod->get_referrable_name()) + " or its submodules",
			           name.range);
		}
	}
	auto const& skName = names.back();
	if (mod->has_skill(skName.value, access) || mod->has_brought_skill(skName.value, access) ||
	    mod->has_skill_in_imports(skName.value, access).first) {
		return mod->get_skill(skName.value, access);
	} else {
		ctx->Error("No skill named " + ctx->color(skName.value) + " could be found in the current scope", skName.range);
	}
}

Maybe<usize> PolymorphType::get_type_bitsize(EmitCtx* ctx) const {
	auto ptrTy = llvm::PointerType::get(ctx->irCtx->llctx, ctx->irCtx->dataLayout.value().getProgramAddressSpace());
	return (usize)ctx->irCtx->dataLayout.value().getTypeAllocSizeInBits(
	    isTyped ? llvm::StructType::create({ptrTy, ptrTy, ptrTy}) : llvm::StructType::create({ptrTy, ptrTy}));
}

ir::Type* PolymorphType::emit(EmitCtx* ctx) {
	Vec<ir::Skill*> irSkills;
	for (auto* sk : skills) {
		irSkills.push_back(sk->find(ctx));
	}
	auto markOwner =
	    ownRange.has_value() ? get_mark_owner(ctx, ownType, None, ownRange.value()) : ir::MarkOwner::of_anonymous();
	return ir::Polymorph::create(isTyped, std::move(irSkills), markOwner, ctx->irCtx);
}

} // namespace qat::ast
