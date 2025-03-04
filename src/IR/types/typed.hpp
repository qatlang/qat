#ifndef QAT_IR_TYPES_TYPED_HPP
#define QAT_IR_TYPES_TYPED_HPP

#include "./qat_type.hpp"

namespace qat::ir {

// Meant mainly for const expressions
class TypedType : public Type {
	ir::Ctx* ctx;

  public:
	explicit TypedType(ir::Ctx* ctx);

	static TypedType* instance;

	useit static TypedType* get(ir::Ctx* ctx);

	useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;

	useit bool can_be_prerun() const final { return true; }

	useit bool can_be_prerun_generic() const final { return true; }

	useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

	useit TypeKind type_kind() const final { return TypeKind::typed; }

	useit String to_string() const final { return "type"; }
};

} // namespace qat::ir

#endif
