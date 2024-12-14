#include "./emit_ctx.hpp"
#include "../IR/context.hpp"
#include "../cli/config.hpp"
#include "./node.hpp"
#include "./types/generic_abstract.hpp"

namespace qat::ast {

AccessInfo EmitCtx::get_access_info() const {
	Maybe<ir::Type*>      type  = None;
	Maybe<ir::DoneSkill*> skill = None;
	if (has_member_parent()) {
		if (get_member_parent()->is_expanded()) {
			type = get_member_parent()->as_expanded();
		} else {
			skill = get_member_parent()->as_done_skill();
		}
	}
	return AccessInfo(mod, type, skill);
}

String EmitCtx::color(String const& message) const {
	auto* cfg = cli::Config::get();
	return (cfg->color_mode() == cli::ColorMode::none ? "`"
	                                                  : String(colors::bold) + cli::get_color(cli::Color::yellow)) +
	       message +
	       (cfg->color_mode() == cli::ColorMode::none ? "`"
	                                                  : String(colors::reset) + cli::get_color(cli::Color::white));
}

void EmitCtx::genericNameCheck(String const& name, FileRange const& range) {
	if (has_fn() && get_fn()->has_generic_parameter(name)) {
		Error("A generic parameter named " + color(name) + " is present in this function. This will lead to ambiguity.",
		      range);
	} else if (has_member_parent()) {
		if (get_member_parent()->is_expanded() && get_member_parent()->as_expanded()->has_generic_parameter(name)) {
			Error("A generic parameter named " + color(name) + " is present in the parent type " +
			          color(get_member_parent()->as_expanded()->to_string()) + ", so this will lead to ambiguity",
			      range);
		} else if (get_member_parent()->is_done_skill() &&
		           get_member_parent()->as_done_skill()->has_generic_parameter(name)) {
			Error("A generic parameter named " + color(name) + " is present in the parent implementation " +
			          color(get_member_parent()->as_done_skill()->to_string()) + ", so this will lead to ambiguity",
			      range);
		}
	} else if (has_opaque_parent() && get_opaque_parent()->has_generic_parameter(name)) {
		Error("A generic parameter named " + color(name) + " is present in the parent type " +
		          color(get_opaque_parent()->to_string()) + ", so this will lead to ambiguity",
		      range);
	}
}

void EmitCtx::name_check_in_module(const Identifier& name, const String& entityType, Maybe<u64> genericID,
                                   Maybe<u64> opaqueID) {
	auto reqInfo = get_access_info();
	if (mod->has_opaque_type(name.value, reqInfo)) {
		auto* opq = mod->get_opaque_type(name.value, get_access_info());
		if (opq->is_generic()) {
			if (genericID.has_value()) {
				if (opq->get_generic_id().value() == genericID.value()) {
					return;
				}
			}
		} else if (opaqueID.has_value()) {
			if (opq->get_id() == opaqueID.value()) {
				return;
			}
		}
		String tyDesc;
		if (opq->is_subtype_struct()) {
			tyDesc = opq->is_generic() ? "generic core type" : "core type";
		} else if (opq->is_subtype_mix()) {
			tyDesc = opq->is_generic() ? "generic mix type" : "mix type";
		} else {
			tyDesc = opq->is_generic() ? "generic type" : "type";
		}
		Error("A " + tyDesc + " named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_struct_type(name.value, reqInfo)) {
		Error("A core type named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_struct_type(name.value, None)) {
		Error("A core type named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_struct_type_in_imports(name.value, reqInfo).first) {
		Error("A core type named " + color(name.value) + " is present inside the module " +
		          color(mod->has_struct_type_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_generic_struct_type(name.value, reqInfo)) {
		if (genericID.has_value() &&
		    mod->get_generic_struct_type(name.value, get_access_info())->get_id() == genericID.value()) {
			return;
		}
		Error("A generic core type named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_generic_struct_type(name.value, None)) {
		if (genericID.has_value() &&
		    mod->get_generic_struct_type(name.value, get_access_info())->get_id() == genericID.value()) {
			return;
		}
		Error("A generic core type named " + color(name.value) +
		          " is brought into this module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_generic_struct_type_in_imports(name.value, reqInfo).first) {
		Error("A generic core type named " + color(name.value) + " is present inside the module " +
		          color(mod->has_generic_struct_type_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_mix_type(name.value, reqInfo)) {
		Error("A mix type named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_mix_type(name.value, None)) {
		Error("A mix type named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_mix_type_in_imports(name.value, reqInfo).first) {
		Error("A mix type named " + color(name.value) + " is present inside the module " +
		          color(mod->has_mix_type_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_choice_type(name.value, reqInfo)) {
		Error("A choice type named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_choice_type(name.value, None)) {
		Error("A choice type named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_choice_type_in_imports(name.value, reqInfo).first) {
		Error("A choice type named " + color(name.value) + " is present inside the module " +
		          color(mod->has_choice_type_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_type_definition(name.value, reqInfo)) {
		Error("A type definition named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_type_definition(name.value, None)) {
		Error("A type definition named " + color(name.value) +
		          " is brought into this module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_type_definition_in_imports(name.value, reqInfo).first) {
		Error("A type definition named " + color(name.value) + " is present inside the module " +
		          color(mod->has_type_definition_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_function(name.value, reqInfo)) {
		Error("A function named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_function(name.value, None)) {
		Error("A function named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_function_in_imports(name.value, reqInfo).first) {
		Error("A function named " + color(name.value) + " is present inside the module " +
		          color(mod->has_function_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_prerun_function(name.value, reqInfo)) {
		Error("A prerun function named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_prerun_function(name.value, None)) {
		Error("A prerun function named " + color(name.value) +
		          " is brought into this module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_prerun_function_in_imports(name.value, reqInfo).first) {
		Error("A prerun function named " + color(name.value) + " is present inside the module " +
		          color(mod->has_function_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_generic_function(name.value, reqInfo)) {
		if (genericID.has_value() &&
		    mod->get_generic_function(name.value, get_access_info())->get_id() == genericID.value()) {
			return;
		}
		Error("A generic function named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_generic_function(name.value, None)) {
		if (genericID.has_value() &&
		    mod->get_generic_function(name.value, get_access_info())->get_id() == genericID.value()) {
			return;
		}
		Error("A generic function named " + color(name.value) +
		          " is brought into this module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_generic_function_in_imports(name.value, reqInfo).first) {
		if (genericID.has_value() &&
		    mod->get_generic_function(name.value, get_access_info())->get_id() == genericID.value()) {
			return;
		}
		Error("A generic function named " + color(name.value) + " is present inside the module " +
		          color(mod->has_generic_function_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_global(name.value, reqInfo)) {
		Error("A global named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_global(name.value, None)) {
		Error("A global named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_global_in_imports(name.value, reqInfo).first) {
		Error("A global named " + color(name.value) + " is present inside the module " +
		          color(mod->has_global_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_prerun_global(name.value, reqInfo)) {
		Error("A prerun global named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_prerun_global(name.value, None)) {
		Error("A prerun global named " + color(name.value) +
		          " is brought into this module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_prerun_global_in_imports(name.value, reqInfo).first) {
		Error("A prerun global named " + color(name.value) + " is present inside the module " +
		          color(mod->has_global_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_region(name.value, reqInfo)) {
		Error("A region named " + color(name.value) + " exists in this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_region(name.value, None)) {
		Error("A region named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_region_in_imports(name.value, reqInfo).first) {
		Error("A region named " + color(name.value) + " is present inside the module " +
		          color(mod->has_region_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_lib(name.value, reqInfo)) {
		Error("A lib named " + color(name.value) + " exists in this module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_brought_lib(name.value, None)) {
		Error("A lib named " + color(name.value) + " is brought into this module. Please change name of this " +
		          entityType + " or check the codebase for inconsistencies",
		      name.range);
	} else if (mod->has_lib_in_imports(name.value, reqInfo).first) {
		Error("A lib named " + color(name.value) + " is present inside the module " +
		          color(mod->has_region_in_imports(name.value, reqInfo).second) +
		          " which is brought into the current module. Please change name of this " + entityType +
		          " or check the codebase for inconsistencies",
		      name.range);
	}
}

VisibilityInfo EmitCtx::get_visibility_info(Maybe<ast::VisibilitySpec> spec) {
	if (spec.has_value() && (spec.value().kind != VisibilityKind::parent)) {
		SHOW("Visibility kind has value")
		switch (spec->kind) {
			case VisibilityKind::lib: {
				if (mod->has_parent_lib()) {
					return VisibilityInfo::lib(mod->get_closest_parent_lib());
				} else {
					Error("The current module does not have a parent lib", spec->range);
				}
			}
			case VisibilityKind::file: {
				return VisibilityInfo::file(mod->get_parent_file());
			}
			case VisibilityKind::folder: {
				auto folderPath = fs::canonical(fs::path(mod->get_file_path()).parent_path());
				if (!mod->has_folder_module(folderPath)) {
					Error("Could not find folder module with path: " + color(folderPath.string()), spec->range);
				}
				return VisibilityInfo::folder(mod->get_folder_module(folderPath));
			}
			case VisibilityKind::skill: {
				if (has_member_parent() && get_member_parent()->is_expanded()) {
					return VisibilityInfo::skill(get_member_parent()->as_expanded());
				} else {
					Error("There is no parent type in the scope and hence this visibility is not allowed", spec->range);
				}
			}
			case VisibilityKind::type: {
				if (has_member_parent()) {
					return VisibilityInfo::type(get_member_parent()->get_parent_type());
				} else {
					Error("There is no parent type and hence " + color("type") + " visibility cannot be used here",
					      spec->range);
				}
			}
			case VisibilityKind::pub: {
				return VisibilityInfo::pub();
			}
			default:
				break;
		}
	} else {
		if (has_member_parent() && get_member_parent()->is_expanded()) {
			return VisibilityInfo::type(get_member_parent()->as_expanded());
		} else if ((has_member_parent() && get_member_parent()->is_done_skill()) || has_skill()) {
			return VisibilityInfo::pub();
		} else {
			switch (mod->get_mod_type()) {
				case ir::ModuleType::file: {
					return VisibilityInfo::file(mod);
				}
				case ir::ModuleType::lib: {
					return VisibilityInfo::lib(mod);
				}
				case ir::ModuleType::folder: {
					return VisibilityInfo::folder(mod);
				}
			}
		}
	}
	SHOW("No visibility info found")
} // NOLINT(clang-diagnostic-return-type)

void EmitCtx::Error(const String& message, Maybe<FileRange> fileRange, Maybe<Pair<String, FileRange>> pointTo) {
	irCtx->Error(mod, message, fileRange, pointTo);
}

bool EmitCtx::has_generic_with_name(String const& name) const {
	for (auto it = generics.rbegin(); it != generics.rend(); it++) {
		if ((*it)->get_name().value == name) {
			return true;
		}
	}
	return false;
}

GenericAbstractType* EmitCtx::get_generic_with_name(String const& name) const {
	for (auto it = generics.rbegin(); it != generics.rend(); it++) {
		if ((*it)->get_name().value == name) {
			return *it;
		}
	}
	return nullptr;
}

} // namespace qat::ast
