#ifndef QAT_IR_CHOICE_HPP
#define QAT_IR_CHOICE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/mentionable.hpp"
#include "../../utils/qat_region.hpp"
#include "../../utils/visibility.hpp"
#include "../meta_info.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class Mod;

class ChoiceType : public Type, public Mentionable {
  private:
	Identifier                     name;
	Mod*                           parent;
	bool                           hasNoneVariant;
	Vec<Vec<Identifier>>           fields;
	Maybe<Vec<llvm::ConstantInt*>> values;
	Maybe<ir::Type*>               providedType;
	bool                           areValuesUnsigned;
	VisibilityInfo                 visibility;
	Maybe<usize>                   defaultVal;
	Maybe<MetaInfo>                metaInfo;

	ir::Ctx*    irCtx;
	mutable u64 bitwidth = 1;

	FileRangePtr fileRange;

  public:
	ChoiceType(Identifier name, Mod* parent, bool hasNoneVariant, Vec<Vec<Identifier>> fields,
	           Maybe<Vec<llvm::ConstantInt*>> values, Maybe<ir::Type*> providedType, bool areValuesUnsigned,
	           Maybe<usize> defaultVal, const VisibilityInfo& visibility, ir::Ctx* irCtx, FileRangePtr fileRange,
	           Maybe<MetaInfo> metaInfo);

	static ChoiceType* create(Identifier name, Mod* parent, bool hasNoneVariant, Vec<Vec<Identifier>> fields,
	                          Maybe<Vec<llvm::ConstantInt*>> values, Maybe<ir::Type*> providedType,
	                          bool areValuesUnsigned, Maybe<usize> defaultVal, const VisibilityInfo& visibility,
	                          ir::Ctx* irCtx, FileRangePtr fileRange, Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(ChoiceType), std::move(name), parent, hasNoneVariant, std::move(fields),
		                         std::move(values), providedType, areValuesUnsigned, defaultVal, visibility, irCtx,
		                         fileRange, std::move(metaInfo));
	}

	Identifier get_name() const;

	String get_full_name() const;

	Mod* get_module() const;

	bool has_custom_value() const;

	bool has_provided_type() const;

	bool has_negative_values() const;

	bool has_none_variant() const { return hasNoneVariant; }

	bool has_default() const;

	bool has_field(const String& name) const;

	Vec<Identifier> const& get_variant_names(String const& name) const {
		for (auto const& it : fields) {
			for (auto& vName : it) {
				if (vName.value == name) {
					return it;
				}
			}
		}
		std::unreachable();
	}

	llvm::ConstantInt* get_value_for(const String& name) const;

	llvm::ConstantInt* get_value_at(usize index) const;

	usize get_variant_count() const { return fields.size(); }

	llvm::ConstantInt* get_default() const;

	ir::Type* get_provided_type() const;

	ir::Type* get_underlying_type() const;

	TypeKind type_kind() const final { return TypeKind::CHOICE; }

	const VisibilityInfo& get_visibility() const;

	void find_bitwidth_normal() const;

	void find_bitwidth_for_values() const;

	void get_missing_names(Vec<Identifier>& vals, Vec<Identifier>& missing) const;

	bool is_type_sized() const final;

	bool has_simple_copy() const final { return true; }

	bool has_simple_move() const final { return true; }

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	String to_string() const final;
};

} // namespace qat::ir

#endif
