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

#include <llvm/IR/LLVMContext.h>
#include <string>
#include <utility>
#include <vector>

namespace qat::ast {
class DefineCoreType;
class Expression;
} // namespace qat::ast

namespace qat::ir {

class StructField final : public EntityOverview, public Uniq {
  public:
	StructField(Identifier _name, Type* _type, bool _variability, Maybe<ast::Expression*> _defVal,
	            const VisibilityInfo& _visibility)
	    : EntityOverview("coreTypeMember",
	                     Json()
	                         ._("name", _name.value)
	                         ._("type", _type->to_string())
	                         ._("typeID", _type->get_id())
	                         ._("isVariable", _variability)
	                         ._("visibility", _visibility),
	                     _name.range),
	      name(std::move(_name)), type(_type), defaultValue(_defVal), visibility(_visibility),
	      variability(_variability) {}

	useit static StructField* create(Identifier name, Type* type, bool variability, Maybe<ast::Expression*> defaultVal,
	                                 const VisibilityInfo& visibility) {
		return std::construct_at(OwnNormal(StructField), name, type, variability, defaultVal, visibility);
	}

	~StructField() final = default;

	Identifier              name;
	Type*                   type;
	Maybe<ast::Expression*> defaultValue;
	VisibilityInfo          visibility;
	bool                    variability;
};

class StructType final : public ExpandedType, public EntityOverview {
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

	useit static StructType* create(Mod* mod, Identifier _name, Vec<GenericArgument*> _generics,
	                                ir::OpaqueType* _opaqued, Vec<StructField*> _members,
	                                const VisibilityInfo& _visibility, llvm::LLVMContext& llctx,
	                                Maybe<MetaInfo> metaInfo, bool isPacked) {
		return std::construct_at(OwnNormal(StructType), mod, std::move(_name), std::move(_generics), _opaqued,
		                         std::move(_members), _visibility, llctx, std::move(metaInfo), isPacked);
	}

	~StructType() final;

	useit Maybe<usize> get_index_of(const String& member) const;
	useit bool         has_field_with_name(const String& member) const;
	useit StructField* get_field_with_name(const String& name) const;
	useit u64          get_field_count() const;
	useit StructField* get_field_at(u64 index);
	useit usize        get_field_index(String const& name) const;
	useit String       get_field_name_at(u64 index) const;
	useit Type*        get_type_of_field(const String& member) const;
	useit Vec<StructField*>& get_members();
	useit bool               has_static(const String& name) const;
	useit bool               is_type_sized() const final;

	useit bool is_trivially_copyable() const final;
	useit bool is_trivially_movable() const final;
	useit bool is_copy_constructible() const final;
	useit bool is_copy_assignable() const final;
	useit bool is_move_constructible() const final;
	useit bool is_move_assignable() const final;
	useit bool is_destructible() const final;

	useit bool can_be_prerun() const final {
		for (auto* mem : members) {
			if (!mem->type->can_be_prerun()) {
				return false;
			}
		}
		return true;
	}

	useit bool can_be_prerun_generic() const final {
		for (auto* mem : members) {
			if (!mem->type->can_be_prerun_generic()) {
				return false;
			}
		}
		return true;
	}

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* value) const final {
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

	useit LinkNames get_link_names() const final;
	useit TypeKind  type_kind() const override;
	useit String    to_string() const override;
	void            addStaticMember(const Identifier& name, Type* type, bool variability, Value* initial,
	                                const VisibilityInfo& visibility, llvm::LLVMContext& llctx);
	void            update_overview() final;
};

class GenericStructType : public Uniq, public EntityOverview {
	friend ast::DefineCoreType;

  private:
	Identifier                     name;
	Vec<ast::GenericAbstractType*> generics;
	ast::DefineCoreType*           defineStructType;
	Mod*                           parent;
	VisibilityInfo                 visibility;
	ast::PrerunExpression*         constraint;

	Vec<String> variantNames;

	mutable Vec<GenericVariant<StructType>>   variants;
	mutable Deque<GenericVariant<OpaqueType>> opaqueVariants;

  public:
	GenericStructType(Identifier name, Vec<ast::GenericAbstractType*> generics, ast::PrerunExpression* _constraint,
	                  ast::DefineCoreType* defineCoreType, Mod* parent, const VisibilityInfo& visibInfo);

	useit static GenericStructType* create(Identifier name, Vec<ast::GenericAbstractType*> generics,
	                                       ast::PrerunExpression* _constraint, ast::DefineCoreType* defineCoreType,
	                                       Mod* parent, const VisibilityInfo& visibInfo) {
		return std::construct_at(OwnNormal(GenericStructType), std::move(name), std::move(generics), _constraint,
		                         defineCoreType, parent, visibInfo);
	}

	~GenericStructType() {
		for (auto& it : variants) {
			it.clear_fill_types();
		}
		for (auto& it : opaqueVariants) {
			it.clear_fill_types();
		}
	}

	useit Identifier get_name() const;
	useit usize      getTypeCount() const;
	useit bool       allTypesHaveDefaults() const;
	useit usize      getVariantCount() const;
	useit Mod*       get_module() const;
	useit Type*      fill_generics(Vec<ir::GenericToFill*>& types, ir::Ctx* irCtx, FileRange range);

	useit ast::GenericAbstractType* getGenericAt(usize index) const;
	useit VisibilityInfo            get_visibility() const;
	void                            update_overview() final;
};

} // namespace qat::ir

#endif
