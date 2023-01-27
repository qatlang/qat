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

  useit Maybe<bool> equalityOf(IR::ConstantValue* first, IR::ConstantValue* second) const final;
  useit bool        canBeConstGeneric() const final;
  useit Maybe<String> toConstGenericString(IR::ConstantValue* constant) const final;

  useit TypeKind typeKind() const final;
  useit String   toString() const final;
  useit Json     toJson() const final { return {}; }
};

} // namespace qat::IR

#endif