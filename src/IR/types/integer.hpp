#ifndef QAT_IR_TYPES_INTEGER_HPP
#define QAT_IR_TYPES_INTEGER_HPP

#include "../../IR/context.hpp"
#include "qat_type.hpp"
#include "type_kind.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

namespace qat {
namespace IR {

class IntegerType : public QatType {
private:
  const unsigned int bitWidth;

public:
  IntegerType(llvm::LLVMContext &ctx, const unsigned int _bitWidth);

  bool isBitWidth(const unsigned int width) const;

  TypeKind typeKind() const;
};
} // namespace IR
} // namespace qat

#endif