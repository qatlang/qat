#ifndef QAT_IR_TYPES_POINTER_HPP
#define QAT_IR_TYPES_POINTER_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>

namespace qat::IR {

/**
 *  A pointer type in the language
 *
 */
class PointerType : public QatType {
private:
  QatType *subType;

  PointerType(QatType *_type, llvm::LLVMContext &ctx);

public:
  useit static PointerType *get(QatType *_type, llvm::LLVMContext &ctx);

  useit QatType *getSubType() const;
  useit TypeKind typeKind() const override;
  useit String   toString() const override;
  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif