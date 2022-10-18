#ifndef QAT_IR_FUTURE_HPP
#define QAT_IR_FUTURE_HPP

#include "qat_type.hpp"
#include "type_kind.hpp"
#include "llvm/IR/LLVMContext.h"

namespace qat::IR {

class FutureType : public QatType {
private:
  QatType* subTy;

  FutureType(QatType* subType, llvm::LLVMContext& ctx);

public:
  useit static FutureType* get(QatType* subType, llvm::LLVMContext& ctx);

  useit QatType* getSubType() const;
  useit String   toString() const final;
  useit TypeKind typeKind() const final;
  useit Json     toJson() const final { return {}; }
};

} // namespace qat::IR

#endif