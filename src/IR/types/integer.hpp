#ifndef QAT_IR_TYPES_INTEGER_HPP
#define QAT_IR_TYPES_INTEGER_HPP

#include "./qat_type.hpp"

namespace qat::IR {

class IntegerType : public QatType {
private:
  const u32 bitWidth;

public:
  IntegerType(const u32 _bitWidth);

  bool isBitWidth(const u32 width) const;

  u64 getBitwidth() const;

  TypeKind typeKind() const;

  String toString() const;
};

} // namespace qat::IR

#endif