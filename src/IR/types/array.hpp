#ifndef QAT_IR_TYPES_ARRAY_HPP
#define QAT_IR_TYPES_ARRAY_HPP

#include "../../utils/types.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/DerivedTypes.h"

namespace qat {
namespace IR {

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
  ArrayType(QatType *_element_type, const uint64_t _length);

  u64 getLength() const;

  TypeKind typeKind() const;
};

} // namespace IR
} // namespace qat

#endif