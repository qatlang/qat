#ifndef QAT_IR_TYPES_TYPED_HPP
#define QAT_IR_TYPES_TYPED_HPP

#include "qat_type.hpp"

namespace qat::ir {

// Meant mainly for const expressions
class TypedType : public Type {
  Type* subTy;

public:
  explicit TypedType(Type* _subTy);

  useit static TypedType* get(Type* _subTy);

  useit Type* get_subtype() const;

  useit Maybe<bool> equality_of(ir::Ctx* irCtx, ir::PrerunValue* first, ir::PrerunValue* second) const final;
  useit inline bool can_be_prerun() const final { return true; }
  useit inline bool can_be_prerun_generic() const final { return true; }
  useit Maybe<String> to_prerun_generic_string(ir::PrerunValue* val) const final;

  useit TypeKind type_kind() const final;
  useit String   to_string() const final;
};

} // namespace qat::ir

#endif
