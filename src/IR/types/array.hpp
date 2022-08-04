#ifndef QAT_IR_TYPES_ARRAY_HPP
#define QAT_IR_TYPES_ARRAY_HPP

#include "../../utils/helpers.hpp"
#include "./qat_type.hpp"

namespace qat::IR {

/**
 *  A continuous sequence of elements of a particular type. The sequence
 * is fixed length
 *
 */
class ArrayType : public QatType {
private:
  QatType *element_type;

  u64 length;

public:
  ArrayType(QatType *_element_type, u64 _length);

  QatType *getElementType();

  useit u64 getLength() const;

  useit TypeKind typeKind() const override;

  useit String toString() const override;

  useit llvm::Type *emitLLVM(llvmHelper &help) const override;

  void emitCPP(cpp::File &file) const override;
};

} // namespace qat::IR

#endif