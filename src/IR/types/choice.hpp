#ifndef QAT_IR_CHOICE_HPP
#define QAT_IR_CHOICE_HPP

#include "../../utils/file_range.hpp"
#include "../../utils/identifier.hpp"
#include "../../utils/qat_region.hpp"
#include "../../utils/visibility.hpp"
#include "../entity_overview.hpp"
#include "../meta_info.hpp"
#include "./qat_type.hpp"

#include <llvm/IR/Constants.h>
#include <llvm/IR/LLVMContext.h>

namespace qat::ir {

class Mod;

class ChoiceType : public Type, public EntityOverview {
  private:
	Identifier                     name;
	Mod*                           parent;
	Vec<Vec<Identifier>>           fields;
	Maybe<Vec<llvm::ConstantInt*>> values;
	Maybe<ir::Type*>               providedType;
	bool                           areValuesUnsigned;
	VisibilityInfo                 visibility;
	Maybe<usize>                   defaultVal;
	Maybe<MetaInfo>                metaInfo;

	ir::Ctx*    irCtx;
	mutable u64 bitwidth = 1;

	FileRange fileRange;

  public:
	ChoiceType(Identifier name, Mod* parent, Vec<Vec<Identifier>> fields, Maybe<Vec<llvm::ConstantInt*>> values,
	           Maybe<ir::Type*> providedType, bool areValuesUnsigned, Maybe<usize> defaultVal,
	           const VisibilityInfo& visibility, ir::Ctx* irCtx, FileRange fileRange, Maybe<MetaInfo> metaInfo);

	useit static ChoiceType* create(Identifier name, Mod* parent, Vec<Vec<Identifier>> fields,
	                                Maybe<Vec<llvm::ConstantInt*>> values, Maybe<ir::Type*> providedType,
	                                bool areValuesUnsigned, Maybe<usize> defaultVal, const VisibilityInfo& visibility,
	                                ir::Ctx* irCtx, FileRange fileRange, Maybe<MetaInfo> metaInfo) {
		return std::construct_at(OwnNormal(ChoiceType), std::move(name), parent, std::move(fields), std::move(values),
		                         providedType, areValuesUnsigned, defaultVal, visibility, irCtx, std::move(fileRange),
		                         std::move(metaInfo));
	}

	useit Identifier get_name() const;

	useit String get_full_name() const;

	useit Mod* get_module() const;

	useit bool has_custom_value() const;

	useit bool has_provided_type() const;

	useit bool has_negative_values() const;

	useit bool has_default() const;

	useit bool has_field(const String& name) const;

	useit llvm::ConstantInt* get_value_for(const String& name) const;

	useit llvm::ConstantInt* get_default() const;

	useit ir::Type* get_provided_type() const;

	useit ir::Type* get_underlying_type() const;

	useit TypeKind type_kind() const final { return TypeKind::CHOICE; }

	useit const VisibilityInfo& get_visibility() const;

	void find_bitwidth_normal() const;

	void find_bitwidth_for_values() const;

	void get_missing_names(Vec<Identifier>& vals, Vec<Identifier>& missing) const;

	void update_overview() final;

	useit bool is_type_sized() const final;

	useit bool has_simple_copy() const final { return true; }

	useit bool has_simple_move() const final { return true; }

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	useit String to_string() const final;
};

} // namespace qat::ir

#endif
