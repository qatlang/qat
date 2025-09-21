#ifndef QAT_IR_TYPES_ERROR_HPP
#define QAT_IR_TYPES_ERROR_HPP

#include "../../utils/qat_region.hpp"
#include "./qat_type.hpp"

namespace qat::ir {

class ErrorType final : public Type {
	bool  hasNoneVariant;
	Type* subType;

	static Vec<ErrorType*> allErrorTypes;

  public:
	ErrorType(bool _hasNoneVariant, Type* _subType) : hasNoneVariant(_hasNoneVariant), subType(_subType) {
		linkingName = (hasNoneVariant ? "qat'error:[" : "qat'error![") + subType->get_name_for_linking() + "]";
		llvmType    = subType->get_llvm_type();
		allErrorTypes.push_back(this);
	}

	useit static ErrorType* get(bool hasNoneVariant, Type* subType) {
		for (auto typ : allErrorTypes) {
			if ((typ->hasNoneVariant == hasNoneVariant) && typ->get_subtype()->is_same(subType)) {
				return typ;
			}
		}
		return std::construct_at(OwnNormal(ErrorType), hasNoneVariant, subType);
	}

	useit Type* get_subtype() const { return subType; }

	useit bool has_none_variant() const { return hasNoneVariant; }

	useit bool can_be_prerun() const final { return subType->can_be_prerun(); }

	useit bool has_prerun_default_value() const final { return has_simple_move(); }

	useit bool is_default_constructible() const final { return hasNoneVariant or subType->is_default_constructible(); }

	useit bool is_copy_constructible() const final { return subType->is_copy_constructible(); }

	useit bool is_copy_assignable() const { return subType->is_copy_assignable(); }

	useit bool is_move_constructible() const { return subType->is_move_constructible(); }

	useit bool is_move_assignable() const { return subType->is_move_assignable(); }

	useit bool is_destructible() const { return subType->is_destructible(); }

	useit ir::PrerunValue* get_prerun_default_value(ir::Ctx* irCtx) final;

	useit bool has_simple_copy() const final { return subType->has_simple_copy(); }

	useit bool has_simple_move() const final { return subType->has_simple_move(); }

	useit bool can_be_prerun_generic() const { return subType->can_be_prerun_generic(); }

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit bool is_type_sized() const final { return true; }

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	void default_construct_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final;

	void copy_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final {
		return subType->copy_construct_value(irCtx, first, second, fun);
	}

	void copy_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final {
		return subType->copy_assign_value(irCtx, first, second, fun);
	}

	void move_construct_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final {
		return subType->move_construct_value(irCtx, first, second, fun);
	}

	void move_assign_value(ir::Ctx* irCtx, ir::Value* first, ir::Value* second, ir::Function* fun) final {
		return subType->move_assign_value(irCtx, first, second, fun);
	}

	void destroy_value(ir::Ctx* irCtx, ir::Value* instance, ir::Function* fun) final {
		return subType->destroy_value(irCtx, instance, fun);
	}

	useit TypeKind type_kind() const final { return TypeKind::ERROR; }

	useit String to_string() const final {
		return (hasNoneVariant ? "error:[" : "error![") + subType->to_string() + "]";
	}
};

} // namespace qat::ir

#endif
