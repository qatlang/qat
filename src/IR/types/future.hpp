#ifndef QAT_IR_FUTURE_HPP
#define QAT_IR_FUTURE_HPP

#include "../context.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"

namespace qat::IR {

class FutureType : public QatType {
private:
  QatType* subTy;

  llvm::Function* destructor = nullptr;

  FutureType(QatType* subType, IR::Context* ctx);

public:
  useit static FutureType* get(QatType* subType, IR::Context* ctx);

  useit QatType* getSubType() const;
  useit String   toString() const final;
  useit TypeKind typeKind() const final;
  useit Json     toJson() const final { return {}; }
};

} // namespace qat::IR

#endif