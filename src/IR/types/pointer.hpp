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
  bool     isSubtypeVar;
  PointerType(bool _isSubVar, QatType *_type, llvm::LLVMContext &ctx);

public:
  useit static PointerType *get(bool _isSubtypeVariable, QatType *_type,
                                llvm::LLVMContext &ctx);

  useit QatType *getSubType() const;
  useit bool     isSubtypeVariable() const;
  useit TypeKind typeKind() const override;
  useit String   toString() const override;
  useit Json     toJson() const override;
};

} // namespace qat::IR

#endif