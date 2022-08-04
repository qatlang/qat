#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "./qat_type.hpp"

namespace qat::IR {

class VoidType : public QatType {
public:
  VoidType() = default;

  useit TypeKind typeKind() const override;

  useit String toString() const override;

  useit llvm::Type *emitLLVM(llvmHelper &help) const override;

  void emitCPP(cpp::File &file) const override;

  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif