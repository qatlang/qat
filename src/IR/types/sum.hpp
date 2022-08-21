#ifndef QAT_IR_TYPES_SUM_TYPE_HPP
#define QAT_IR_TYPES_SUM_TYPE_HPP

#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include <string>
#include <vector>

namespace qat::IR {

class QatModule;

class SumType : public QatType {
private:
  String         name;
  Vec<QatType *> subTypes;
  QatModule     *parent;
  mutable u64    tagBitwidth = 8; // NOLINT(readability-magic-numbers)

public:
  SumType(String _name, Vec<QatType *> _subTypes, llvm::LLVMContext &llctx);

  useit String   getName() const;
  useit String   getFullName() const;
  useit u32      getSubtypeCount() const;
  useit QatType *getSubtypeAt(usize index);
  useit bool     isCompatible(QatType *other);
  useit TypeKind typeKind() const override { return TypeKind::sumType; }
  useit String   toString() const override;
  useit nuo::Json toJson() const override;
};

} // namespace qat::IR

#endif