#ifndef QAT_IR_TYPES_CSTRING_HPP
#define QAT_IR_TYPES_CSTRING_HPP

#include "qat_type.hpp"

namespace qat::IR {

class CStringType : public QatType {
private:
  CStringType(llvm::LLVMContext &ctx);

public:
  useit static CStringType *get(llvm::LLVMContext &ctx);
  useit TypeKind            typeKind() const override;
  useit String              toString() const override;
  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif