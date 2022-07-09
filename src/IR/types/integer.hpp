#ifndef QAT_IR_TYPES_INTEGER_HPP
#define QAT_IR_TYPES_INTEGER_HPP

#include "./qat_type.hpp"

namespace qat {
namespace IR {

class IntegerType : public QatType {
private:
  const unsigned int bitWidth;

public:
  IntegerType(const unsigned int _bitWidth);

  bool isBitWidth(const unsigned int width) const;

  unsigned getBitwidth() const;

  TypeKind typeKind() const;
};
} // namespace IR
} // namespace qat

#endif