#ifndef QAT_IR_TYPES_ARRAY_HPP
#define QAT_IR_TYPES_ARRAY_HPP

#include "../../utils/types.hpp"
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
  ArrayType(QatType *_element_type, const uint64_t _length);

  QatType *getElementType();

  u64 getLength() const;

  TypeKind typeKind() const;

  std::string toString() const;
};

} // namespace qat::IR

#endif