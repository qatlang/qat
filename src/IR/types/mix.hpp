#ifndef QAT_IR_MIX_TYPE_HPP
#define QAT_IR_MIX_TYPE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../generics.hpp"
#include "./expanded_type.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class Mod;

class MixType : public ExpandedType, public EntityOverview {
  private:
	Vec<Pair<Identifier, Maybe<Type*>>> subtypes;

	u64	 maxSize	   = 8u;
	bool isPack		   = false;
	bool isTrivialCopy = true;
	bool isTrivialMove = true;

	usize			tagBitWidth = 1;
	Maybe<usize>	defaultVal;
	FileRange		fileRange;
	Maybe<MetaInfo> metaInfo;

	ir::OpaqueType* opaquedType = nullptr;

	void findTagBitWidth();

  public:
	MixType(Identifier name, ir::OpaqueType* opaquedTy, Vec<GenericArgument*> _generics, Mod* parent,
			Vec<Pair<Identifier, Maybe<Type*>>> subtypes, Maybe<usize> defaultVal, ir::Ctx* irCtx, bool isPacked,
			const VisibilityInfo& visibility, FileRange fileRange, Maybe<MetaInfo> metaInfo);

	useit static MixType* create(Identifier name, ir::OpaqueType* opaquedTy, Vec<GenericArgument*> _generics,
								 Mod* parent, Vec<Pair<Identifier, Maybe<Type*>>> subtypes, Maybe<usize> defaultVal,
								 ir::Ctx* irCtx, bool isPacked, const VisibilityInfo& visibility, FileRange fileRange,
								 Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(MixType), std::move(name), opaquedTy, std::move(_generics), parent,
								 std::move(subtypes), defaultVal, irCtx, isPacked, visibility, std::move(fileRange),
								 std::move(metaInfo));
	}

	useit usize get_index_of(const String& name) const;

	useit Pair<bool, bool> has_variant_with_name(const String& sname) const;
	useit Type*			   get_variant_with_name(const String& sname) const;

	useit bool		has_default_variant() const;
	useit usize		get_default_index() const;
	useit usize		get_variant_count() const;
	useit bool		is_packed() const;
	useit usize		get_tag_bitwidth() const;
	useit u64		get_data_bitwidth() const;
	useit FileRange get_file_range() const;
	useit String	to_string() const final;
	useit TypeKind	type_kind() const final;
	useit LinkNames get_link_names() const final;
	useit bool		is_type_sized() const final;

	useit bool can_be_prerun() const final {
		for (auto sub : subtypes) {
			if (sub.second.has_value() && !sub.second.value()->can_be_prerun()) {
				return false;
			}
		}
		return true;
	}

	useit bool is_trivially_copyable() const final;
	useit bool is_trivially_movable() const final;
	useit bool is_copy_constructible() const final;
	useit bool is_copy_assignable() const final;
	useit bool is_move_constructible() const final;
	useit bool is_move_assignable() const final;
	useit bool is_destructible() const final;

	void get_missing_names(Vec<Identifier>& vals, Vec<Identifier>& missing) const;
	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;
	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;
	void update_overview() final;
};

} // namespace qat::ir

#endif
