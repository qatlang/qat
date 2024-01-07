#ifndef QAT_IR_TYPES_STRING_SLICE_HPP
#define QAT_IR_TYPES_STRING_SLICE_HPP

#include "qat_type.hpp"

namespace qat::IR {

class StringSliceType : public QatType {
private:
  bool isPack;

  StringSliceType(IR::Context* ctx, bool isPacked = false);

public:
  useit static StringSliceType* get(IR::Context* ctx, bool isPacked = false);
  useit static IR::PrerunValue* Create(IR::Context* ctx, String value);
  useit bool                    isPacked() const;
  useit TypeKind                typeKind() const override;
  useit String                  toString() const override;

  useit bool canBePrerun() const final { return true; }
  useit bool canBePrerunGeneric() const final { return true; }
  useit bool isTypeSized() const final;
  useit bool isTriviallyCopyable() const final { return true; }
  useit bool isTriviallyMovable() const final { return true; }
  useit Maybe<String> toPrerunGenericString(IR::PrerunValue* val) const final;
  useit Maybe<bool> equalityOf(IR::Context* ctx, IR::PrerunValue* first, IR::PrerunValue* second) const final;
};

} // namespace qat::IR

#endif