#ifndef QAT_IR_FUTURE_HPP
#define QAT_IR_FUTURE_HPP

#include "../context.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::IR {

class FutureType : public QatType {
private:
  QatType* subTy;
  bool     isPacked;

  FutureType(QatType* subType, bool isPacked, IR::Context* ctx);

public:
  useit static FutureType* get(QatType* subType, bool isPacked, IR::Context* ctx);

  useit QatType* getSubType() const;
  useit String   toString() const final;
  useit TypeKind typeKind() const final;
  useit bool     isDestructible() const final;
  useit bool     isTypeSized() const final;
  useit bool     isTypePacked() const;
  void           destroyValue(IR::Context* ctx, Vec<IR::Value*> vals, IR::Function* fun) final;
};

} // namespace qat::IR

#endif