#ifndef QAT_IR_MIX_TYPE_HPP
#define QAT_IR_MIX_TYPE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../generics.hpp"
#include "./expanded_type.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class Mod;

class MixType : public ExpandedType {
  private:
	Vec<Pair<Identifier, Maybe<Type*>>> subtypes;

	u64  maxSize = 8u;
	bool isPack  = false;

	usize           tagBitWidth = 1;
	Maybe<usize>    defaultVal;
	FileRangePtr    fileRange;
	Maybe<MetaInfo> metaInfo;
	bool            hasNoneVariant;

	ir::OpaqueType* opaquedType = nullptr;

	void find_tag_bitwidth();

  public:
	MixType(Identifier name, ir::OpaqueType* opaquedTy, Vec<GenericArgument*> _generics, Mod* parent,
	        Vec<Pair<Identifier, Maybe<Type*>>> subtypes, Maybe<usize> defaultVal, ir::Ctx* irCtx, bool addNoneVariant,
	        bool isPacked, const VisibilityInfo& visibility, FileRangePtr fileRange, Maybe<MetaInfo> metaInfo);

	static MixType* create(Identifier name, ir::OpaqueType* opaquedTy, Vec<GenericArgument*> _generics, Mod* parent,
	                       Vec<Pair<Identifier, Maybe<Type*>>> subtypes, Maybe<usize> defaultVal, ir::Ctx* irCtx,
	                       bool addNoneVariant, bool isPacked, const VisibilityInfo& visibility, FileRangePtr fileRange,
	                       Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(MixType), std::move(name), opaquedTy, std::move(_generics), parent,
		                         std::move(subtypes), defaultVal, irCtx, addNoneVariant, isPacked, visibility,
		                         fileRange, std::move(metaInfo));
	}

	usize get_index_of(const String& name) const;

	bool has_none_variant() const { return hasNoneVariant; }

	Pair<bool, bool> has_variant_with_name(const String& sname) const;
	Type*            get_variant_with_name(const String& sname) const;

	bool  has_default_variant() const;
	usize get_default_index() const;

	usize get_variant_count() const;
	usize get_variant_index(String const& name) const;

	Type* get_variant_type_at(usize index) const;

	bool         is_packed() const;
	usize        get_tag_bitwidth() const;
	u64          get_data_bitwidth() const;
	FileRangePtr get_file_range() const;
	String       to_string() const final;
	TypeKind     type_kind() const final;
	LinkNames    get_link_names() const final;
	bool         is_type_sized() const final;

	bool can_be_prerun() const final {
		for (auto sub : subtypes) {
			if (sub.second.has_value() && not sub.second.value()->can_be_prerun()) {
				return false;
			}
		}
		return true;
	}

	void get_missing_names(Vec<Identifier>& vals, Vec<Identifier>& missing) const;
	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;
};

} // namespace qat::ir

#endif
