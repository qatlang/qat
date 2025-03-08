#include "./toggle.hpp"
#include "../context.hpp"

namespace qat::ir {

ToggleType::ToggleType(Identifier _name, Vec<Pair<Identifier, Type*>> _variants, ir::Mod* _parent,
                       VisibilityInfo _visibility, Maybe<MetaInfo> _metaInfo, ir::Ctx* irCtx)
    : ExpandedType(_name, {}, _parent, _visibility), variants(_variants), metaInfo(_metaInfo) {
	usize maxSizeInBytes = 0;

	usize       candidateSizeInBytes = 0;
	usize       candidateAlign       = 1024;
	llvm::Type* candidateType        = nullptr;
	for (auto& it : variants) {
		auto typeSize  = (usize)irCtx->dataLayout.getTypeStoreSize(it.second->get_llvm_type());
		auto typeAlign = irCtx->dataLayout.getPrefTypeAlign(it.second->get_llvm_type()).value();
		if (typeAlign < candidateAlign) {
			candidateSizeInBytes = typeSize;
			candidateAlign       = typeAlign;
			candidateType        = it.second->get_llvm_type();
		}
		if (typeSize > maxSizeInBytes) {
			maxSizeInBytes = typeSize;
		}
	}
	Maybe<String> foreignID;
	Maybe<String> linkAs;
	if (metaInfo.has_value()) {
		if (metaInfo.value().has_key(MetaInfo::linkAsKey)) {
			linkAs = metaInfo.value().get_value_as_string_for(MetaInfo::linkAsKey);
		}
		foreignID = metaInfo.value().get_foreign_id();
	}
	auto linkNames = _parent->get_link_names().newWith(LinkNameUnit(_name.value, LinkUnitType::toggle), foreignID);
	linkingName    = linkNames.toName();
	Vec<llvm::Type*> elements;
	elements.push_back(candidateType);
	if (maxSizeInBytes > candidateSizeInBytes) {
		elements.push_back(
		    llvm::ArrayType::get(llvm::Type::getInt8Ty(irCtx->llctx), maxSizeInBytes - candidateSizeInBytes));
	}
	llvmType = llvm::StructType::create(irCtx->llctx, elements, linkingName);
}

LinkNames ToggleType::get_link_names() const {
	Maybe<String> foreignID;
	Maybe<String> linkAlias;
	if (metaInfo) {
		foreignID = metaInfo->get_foreign_id();
		linkAlias = metaInfo->get_value_as_string_for(ir::MetaInfo::linkAsKey);
	}
	if (not foreignID.has_value()) {
		foreignID = parent->get_relevant_foreign_id();
	}
	auto linkNames = parent->get_link_names().newWith(LinkNameUnit(name.value, LinkUnitType::toggle), foreignID);
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

} // namespace qat::ir
