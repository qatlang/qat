#ifndef QAT_IR_TYPES_REFERENCE_HPP
#define QAT_IR_TYPES_REFERENCE_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>

namespace qat::IR {

class ReferenceType : public QatType {
private:
  QatType *subType;
  ReferenceType(QatType *_type, llvm::LLVMContext &ctx);

public:
  useit static ReferenceType *get(QatType *_subtype, llvm::LLVMContext &ctx);

  useit QatType *getSubType() const;
  useit TypeKind typeKind() const override;
  useit String   toString() const override;
  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif