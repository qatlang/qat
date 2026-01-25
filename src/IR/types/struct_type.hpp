#ifndef QAT_IR_TYPES_STRUCT_TYPE_HPP
#define QAT_IR_TYPES_STRUCT_TYPE_HPP

#include "../../utils/identifier.hpp"
#include "../../utils/qat_region.hpp"
#include "../../utils/visibility.hpp"
#include "../generics.hpp"
#include "../method.hpp"
#include "../static_member.hpp"
#include "./qat_type.hpp"
#include "expanded_type.hpp"

#include <helpers/deque.hpp>
#include <llvm/IR/LLVMContext.h>
#include <utility>

namespace qat::ast {
class DefineStructType;
class Expression;
} // namespace qat::ast

namespace qat::ir {

class StructField final : public Uniq, public Mentionable {
  public:
	StructField(Identifier _name, Type* _type, bool _variability, Maybe<ast::Expression*> _defVal,
	            const VisibilityInfo& _visibility)
	    : name(std::move(_name)), type(_type), defaultValue(_defVal), visibility(_visibility),
	      variability(_variability) {}

	static StructField* create(Identifier name, Type* type, bool variability, Maybe<ast::Expression*> defaultVal,
	                           const VisibilityInfo& visibility) {
		return std::construct_at(OwnNormal(StructField), name, type, variability, defaultVal, visibility);
	}

	~StructField() = default;

	Identifier              name;
	Type*                   type;
	Maybe<ast::Expression*> defaultValue;
	VisibilityInfo          visibility;
	bool                    variability;
};

class StructType final : public ExpandedType {
	friend class StructField;
	friend class Method;
	friend class GenericArgument;
	ir::OpaqueType*    opaquedType = nullptr;
	Vec<StructField*>  members;
	Vec<StaticMember*> staticMembers;
	Maybe<MetaInfo>    metaInfo;

  public:
	StructType(Mod* mod, Identifier _name, Vec<GenericArgument*> _generics, ir::OpaqueType* _opaqued,
	           Vec<StructField*> _members, const VisibilityInfo& _visibility, llvm::LLVMContext& llctx,
	           Maybe<MetaInfo> metaInfo, bool isPacked);

	static StructType* create(Mod* mod, Identifier _name, Vec<GenericArgument*> _generics, ir::OpaqueType* _opaqued,
	                          Vec<StructField*> _members, const VisibilityInfo& _visibility, llvm::LLVMContext& llctx,
	                          Maybe<MetaInfo> metaInfo, bool isPacked) {
		return std::construct_at(OwnNormal(StructType), mod, std::move(_name), std::move(_generics), _opaqued,
		                         std::move(_members), _visibility, llctx, std::move(metaInfo), isPacked);
	}

	~StructType() final;

	Maybe<usize> get_index_of(const String& member) const;

	bool has_field_with_name(const String& member) const;

	StructField* get_field_with_name(const String& name) const;

	u64 get_field_count() const;

	StructField* get_field_at(u64 index);

	usize get_field_index(String const& name) const;

	String get_field_name_at(u64 index) const;

	Type* get_type_of_field(const String& member) const;

	Vec<StructField*>& get_members();

	bool has_static_field(String const& name) const;

	StaticMember* get_static_field(String const& name) const;

	bool is_type_sized() const final;

	bool can_be_prerun() const final {
		for (auto* mem : members) {
			if (not mem->type->can_be_prerun()) {
				return false;
			}
		}
		return true;
	}

	bool can_be_prerun_generic() const final {
		for (auto* mem : members) {
			if (not mem->type->can_be_prerun_generic()) {
				return false;
			}
		}
		return true;
	}

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* value) const final {
		if (can_be_prerun_generic()) {
			auto   valConst = value->get_llvm_constant();
			String result(get_full_name());
			result.append("{ ");
			for (usize i = 0; i < members.size(); i++) {
				result.append(members[i]
				                  ->type
				                  ->to_prerun_generic_string(
				                      ir::PrerunValue::get(valConst->getAggregateElement(i), members[i]->type))
				                  .value());
				if (i != (members.size() - 1)) {
					result.append(", ");
				}
			}
			result.append(" }");
			return result;
		}
		return None;
	}

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final;

	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	LinkNames get_link_names() const final;

	TypeKind type_kind() const override;

	String to_string() const override;

	void add_static_member(const Identifier& name, Type* type, bool variability, Value* initial,
	                       const VisibilityInfo& visibility, llvm::LLVMContext& llctx);
};

class GenericStructType : public Uniq, public Mentionable {
	friend ast::DefineStructType;

  private:
	Identifier                     name;
	Vec<ast::GenericAbstractType*> generics;
	ast::DefineStructType*         defineStructType;
	Mod*                           parent;
	VisibilityInfo                 visibility;
	ast::PrerunExpression*         constraint;

	std::set<String> variantNames;

	mutable Vec<GenericVariant<StructType>>   variants;
	mutable Deque<GenericVariant<OpaqueType>> opaqueVariants;

  public:
	GenericStructType(Identifier name, Vec<ast::GenericAbstractType*> generics, ast::PrerunExpression* _constraint,
	                  ast::DefineStructType* defineStructType, Mod* parent, VisibilityInfo const& visibInfo);

	static GenericStructType* create(Identifier name, Vec<ast::GenericAbstractType*> generics,
	                                 ast::PrerunExpression* _constraint, ast::DefineStructType* defineStructType,
	                                 Mod* parent, const VisibilityInfo& visibInfo) {
		return std::construct_at(OwnNormal(GenericStructType), std::move(name), std::move(generics), _constraint,
		                         defineStructType, parent, visibInfo);
	}

	~GenericStructType() {
		for (auto& it : variants) {
			it.clear_fill_types();
		}
		for (auto& it : opaqueVariants) {
			it.clear_fill_types();
		}
	}

	Identifier get_name() const;

	usize get_parameter_count() const;

	bool all_parameters_have_default() const;

	usize getVariantCount() const;

	Mod* get_module() const;

	Type* fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRangePtr range);

	ast::GenericAbstractType* getGenericAt(usize index) const;

	VisibilityInfo get_visibility() const;
};

} // namespace qat::ir

#endif
