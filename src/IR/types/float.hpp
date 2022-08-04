#ifndef QAT_IR_TYPES_FLOAT_HPP
#define QAT_IR_TYPES_FLOAT_HPP

#include "./qat_type.hpp"

namespace qat::IR {

enum class FloatTypeKind { _brain, _half, _32, _64, _80, _128PPC, _128 };

class FloatType : public QatType {
private:
  FloatTypeKind kind;

public:
  FloatType(FloatTypeKind _kind);

  useit FloatTypeKind getKind() const;
  useit TypeKind      typeKind() const final;
  useit String        toString() const override;
  useit nuo::Json toJson() const override;
  useit llvm::Type *emitLLVM(llvmHelper &help) const override;
  void              emitCPP(cpp::File &file) const override;
};

} // namespace qat::IR

#endif