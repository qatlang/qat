#include "./opaque.hpp"
#include "../../show.hpp"
#include "../context.hpp"
#include "../qat_module.hpp"

#include <llvm/IR/DerivedTypes.h>

namespace qat::ir {

OpaqueType::OpaqueType(Identifier _name, Vec<GenericArgument*> _generics, Maybe<u64> _genericID,
                       Maybe<OpaqueSubtypeKind> _subtypeKind, ir::Mod* _parent, Maybe<usize> _size,
                       VisibilityInfo _visibility, llvm::LLVMContext& llctx, Maybe<MetaInfo> _metaInfo)
    : EntityOverview(
          _subtypeKind.has_value()
              ? (_subtypeKind.value() == OpaqueSubtypeKind::core
                     ? "coreType"
                     : (_subtypeKind.value() == OpaqueSubtypeKind::mix
                            ? "mixType"
                            : (_subtypeKind.value() == OpaqueSubtypeKind::Union ? "unionType" : "opaqueType")))
              : "opaqueType",
          Json(), _name.range),
      name(_name), generics(_generics), genericID(_genericID), subtypeKind(_subtypeKind), parent(_parent), size(_size),
      visibility(_visibility), metaInfo(_metaInfo) {
	Maybe<String> foreignID;
	Maybe<String> linkAlias;
	if (metaInfo.has_value()) {
		foreignID = metaInfo->get_foreign_id();
		linkAlias = metaInfo->get_value_as_string_for(ir::MetaInfo::linkAsKey);
	}
	if (not foreignID.has_value()) {
		foreignID = parent->get_relevant_foreign_id();
	}
	auto linkNames = parent->get_link_names().newWith(
	    LinkNameUnit(name.value, (subtypeKind.has_value() && subtypeKind.value() == OpaqueSubtypeKind::mix)
	                                 ? LinkUnitType::mix
	                                 : (subtypeKind.has_value() && subtypeKind.value() == OpaqueSubtypeKind::Union
	                                        ? LinkUnitType::toggle
	                                        : LinkUnitType::type)),
	    foreignID);
	if (is_generic()) {
		Vec<LinkNames> genericLinkNames;
		for (auto* param : generics) {
			if (param->is_typed()) {
				genericLinkNames.push_back(
				    LinkNames({LinkNameUnit(param->as_typed()->get_type()->get_name_for_linking(),
				                            LinkUnitType::genericTypeValue)},
				              None, nullptr));
			} else if (param->is_prerun()) {
				auto preRes = param->as_prerun();
				genericLinkNames.push_back(LinkNames(
				    {LinkNameUnit(preRes->get_type()->to_prerun_generic_string(preRes->get_expression()).value(),
				                  LinkUnitType::genericPrerunValue)},
				    None, nullptr));
			}
		}
		linkNames.addUnit(LinkNameUnit("", LinkUnitType::genericList, genericLinkNames), None);
	}
	linkNames.setLinkAlias(linkAlias);
	linkingName = linkNames.toName();
	SHOW("Linking name is " << linkingName)
	llvmType = llvm::StructType::create(llctx, linkingName);
	if (not is_generic()) {
		parent->opaqueTypes.push_back(this);
	}
}

OpaqueType* OpaqueType::get(Identifier name, Vec<GenericArgument*> generics, Maybe<u64> genericID,
                            Maybe<OpaqueSubtypeKind> subtypeKind, ir::Mod* parent, Maybe<usize> size,
                            VisibilityInfo visibility, llvm::LLVMContext& llCtx, Maybe<MetaInfo> metaInfo) {
	return std::construct_at(OwnNormal(OpaqueType), name, generics, genericID, subtypeKind, parent, size, visibility,
	                         llCtx, metaInfo);
}

String OpaqueType::get_full_name() const {
	auto result = parent->get_fullname_with_child(name.value);
	if (is_generic()) {
		result += ":[";
		for (usize i = 0; i < generics.size(); i++) {
			result += generics.at(i)->to_string();
			if (i != (generics.size() - 1)) {
				result += ", ";
			}
		}
		result += "]";
	}
	return result;
}

Identifier OpaqueType::get_name() const { return name; }

ir::Mod* OpaqueType::get_module() const { return parent; }

bool OpaqueType::is_generic() const { return not generics.empty(); }

Maybe<u64> OpaqueType::get_generic_id() const { return genericID; }

bool OpaqueType::has_generic_parameter(const String& name) const {
	for (auto* gen : generics) {
		if (gen->is_same(name)) {
			return true;
		}
	}
	return false;
}

GenericArgument* OpaqueType::get_generic_parameter(const String& name) const {
	for (auto* gen : generics) {
		if (gen->is_same(name)) {
			return gen;
		}
	}
	return nullptr;
}

VisibilityInfo const& OpaqueType::get_visibility() const { return visibility; }

bool OpaqueType::is_subtype_struct() const { return subtypeKind == OpaqueSubtypeKind::core; }

bool OpaqueType::is_subtype_mix() const { return subtypeKind == OpaqueSubtypeKind::mix; }

bool OpaqueType::is_subtype_unknown() const { return subtypeKind == OpaqueSubtypeKind::unknown; }

bool OpaqueType::has_subtype() const { return subTy != nullptr; }

void OpaqueType::set_sub_type(ir::ExpandedType* _subTy) {
	subTy = _subTy;
	SHOW("Opaque: set subtype")
	if (not is_generic()) {
		for (auto item = parent->opaqueTypes.begin(); item != parent->opaqueTypes.end(); item++) {
			if ((*item)->get_id() == get_id()) {
				parent->opaqueTypes.erase(item);
				break;
			}
		}
	}
}

ir::Type* OpaqueType::get_subtype() const { return subTy; }

bool OpaqueType::has_deduced_size() const { return size.has_value(); }

usize OpaqueType::get_deduced_size() const { return size.value(); }

bool OpaqueType::is_expanded() const { return subTy && subTy->is_expanded(); }

bool OpaqueType::can_be_prerun() const { return subTy && subTy->can_be_prerun(); }

bool OpaqueType::can_be_prerun_generic() const { return subTy && subTy->can_be_prerun_generic(); }

Maybe<String> OpaqueType::to_prerun_generic_string(ir::PrerunValue* preVal) const {
	if (subTy) {
		return subTy->to_prerun_generic_string(preVal);
	} else {
		// FIXME - Throw error?
		return None;
	}
}

bool OpaqueType::is_type_sized() const { return subTy ? subTy->is_type_sized() : size.has_value(); }

bool OpaqueType::is_trivially_copyable() const { return subTy && subTy->is_trivially_copyable(); }

bool OpaqueType::is_trivially_movable() const { return subTy && subTy->is_trivially_movable(); }

Maybe<bool> OpaqueType::equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const {
	if (subTy) {
		return subTy->equality_of(irCtx, first, second);
	} else {
		return None;
	}
}

bool OpaqueType::is_copy_constructible() const { return subTy && subTy->is_copy_constructible(); }

bool OpaqueType::is_copy_assignable() const { return subTy && subTy->is_copy_assignable(); }

bool OpaqueType::is_move_constructible() const { return subTy && subTy->is_move_constructible(); }

bool OpaqueType::is_move_assignable() const { return subTy && subTy->is_move_assignable(); }

bool OpaqueType::is_destructible() const { return subTy && subTy->is_destructible(); }

void OpaqueType::copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (subTy) {
		subTy->copy_construct_value(irCtx, first, second, fun);
	} else {
		irCtx->Error("Could not copy construct an instance of opaque type " + irCtx->color(to_string()), None);
	}
}

void OpaqueType::copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (subTy) {
		subTy->copy_assign_value(irCtx, first, second, fun);
	} else {
		irCtx->Error("Could not copy assign an instance of opaque type " + irCtx->color(to_string()), None);
	}
}

void OpaqueType::move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (subTy) {
		subTy->move_construct_value(irCtx, first, second, fun);
	} else {
		irCtx->Error("Could not move construct an instance of opaque type " + irCtx->color(to_string()), None);
	}
}

void OpaqueType::move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) {
	if (subTy) {
		subTy->move_assign_value(irCtx, first, second, fun);
	} else {
		irCtx->Error("Could not move assign an instance of opaque type " + irCtx->color(to_string()), None);
	}
}

void OpaqueType::destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) {
	if (subTy) {
		subTy->destroy_value(irCtx, instance, fun);
	} else {
		irCtx->Error("Could not destroy an instance of opaque type " + irCtx->color(to_string()), None);
	}
}

TypeKind OpaqueType::type_kind() const { return TypeKind::OPAQUE; }

String OpaqueType::to_string() const { return get_full_name(); }

} // namespace qat::ir
