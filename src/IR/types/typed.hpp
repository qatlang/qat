#ifndef QAT_IR_TYPES_TYPED_HPP
#define QAT_IR_TYPES_TYPED_HPP

#include "./qat_type.hpp"

namespace qat::ir {

// Meant mainly for const expressions
class TypedType : public Type {
	ir::Ctx* ctx;

  public:
	explicit TypedType(ir::Ctx* ctx);

	static TypedType* typedType;

	static TypedType* get(ir::Ctx* ctx);

	Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	bool can_be_prerun() const final { return true; }

	bool can_be_prerun_generic() const final { return true; }

	Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	TypeKind type_kind() const final { return TypeKind::TYPED; }

	String to_string() const final { return "type"; }
};

} // namespace qat::ir

#endif
