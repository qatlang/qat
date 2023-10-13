#ifndef QAT_IR_TYPES_TYPED_HPP
#define QAT_IR_TYPES_TYPED_HPP

#include "qat_type.hpp"

namespace qat::IR {

// Meant mainly for const expressions
class TypedType : public QatType {
  QatType* subTy;

public:
  explicit TypedType(QatType* _subTy);

  useit static TypedType* get(QatType* _subTy);

  useit QatType* getSubType() const;

  useit Maybe<bool> equalityOf(IR::PrerunValue* first, IR::PrerunValue* second) const final;
  useit bool        canBePrerunGeneric() const final;
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;

  useit TypeKind typeKind() const final;
  useit String   toString() const final;
};

} // namespace qat::IR

#endif
