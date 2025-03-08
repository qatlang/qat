#include "./skill_entity.hpp"

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
		ent->addDependency(
		    ir::EntityDependency{mod->get_entity(skName.value), expect.value_or(ir::DependType::complete), phase});
	}
}

String SkillEntity::to_string() const {
	String result;
	for (auto i = 0; i < relative; i++) {
		result += "up:";
	}
	for (usize i = 0; i < names.size(); i++) {
		result += names[i].value;
		if (i != (names.size() - 1)) {
			result += ":";
		}
	}
	if (not generics.empty()) {
		result += ":[";
		for (usize i = 0; i < generics.size(); i++) {
			result += generics[i]->to_string();
			if (i != (generics.size() - 1)) {
				result += ", ";
			}
		}
		result += "]";
	}
	return result;
}

Json SkillEntity::to_json() const {
	Vec<JsonValue> namesJSON;
	for (auto& id : names) {
		namesJSON.push_back(id);
	}
	Vec<JsonValue> genJSON;
	for (auto* gen : generics) {
		genJSON.push_back(gen->to_json());
	}
	return Json()._("relative", relative)._("names", namesJSON)._("generics", genJSON)._("range", range);
}

ir::Skill* SkillEntity::find_skill(EmitCtx* ctx) const {
	auto mod = ctx->mod;
	if (relative > 0) {
		if (not mod->has_nth_parent(relative)) {
			ctx->Error("The current module " + ctx->color(mod->get_referrable_name()) + " does not have " +
			               ctx->color(std::to_string(relative)) + " parent modules",
			           range);
		}
		mod = mod->get_nth_parent(relative);
	}
	auto access = ctx->get_access_info();
	for (usize i = 0; i < names.size() - 1; i++) {
		auto& idn = names.at(i);
		if (mod->has_lib(idn.value, access) || mod->has_brought_lib(idn.value, access) ||
		    mod->has_lib_in_imports(idn.value, access).first) {
			mod = mod->get_lib(idn.value, access);
			mod->add_mention(idn.range);
			if (not mod->get_visibility().is_accessible(access)) {
				ctx->Error("This lib is not accessible in the current scope", idn.range);
			}
		} else if (mod->has_brought_mod(idn.value, access)) {
			mod = mod->get_brought_mod(idn.value, access);
			mod->add_mention(idn.range);
			if (not mod->get_visibility().is_accessible(access)) {
				ctx->Error("This brought module is not accessible in the current scope", idn.range);
			}
		} else {
			ctx->Error("No lib or brought module named " + ctx->color(idn.value) + " found", idn.range);
		}
	}
	auto& ent = names.back();
	if (generics.empty() && (mod->has_skill(ent.value, access) || mod->has_brought_skill(ent.value, access) ||
	                         mod->has_skill_in_imports(ent.value, access).first)) {
		auto skill = mod->get_skill(ent.value, access);
		skill->add_mention(ent.range);
		return skill;
	} else if ((not generics.empty()) && mod->has_generic_skill(ent.value, access)) {
		auto genSkill = mod->get_generic_skill(ent.value, access);
		// TODO - Add mentions to the generic skill
		Vec<ir::GenericToFill*> toFill;
		for (auto* gen : generics) {
			toFill.push_back(gen->toFill(ctx));
		}
		auto skill = genSkill->fill_generics(toFill, ctx->irCtx, range);
		return skill;
	} else if (generics.empty()) {
		ctx->Error(
		    "Could not find a skill named " + ctx->color(ent.value) + " in the module " +
		        ctx->color(mod->get_referrable_name()) +
		        String(
		            mod->has_generic_skill(ent.value, access)
		                ? (". Found a generic skill with the same name instead in the module. Did you mean to create a variant of the generic skill?")
		                : ""),
		    range);
	} else {
		ctx->Error(
		    "Could not find a generic skill named " + ctx->color(ent.value) + " in the module " +
		        ctx->color(mod->get_referrable_name()) +
		        String(
		            mod->has_skill(ent.value, access)
		                ? ". Found a skill in the module with the same name instead. Did you mean to use that skill, or to create a variant of another generic skill?"
		                : ""),
		    range);
	}
	return nullptr;
}

} // namespace qat::ast
