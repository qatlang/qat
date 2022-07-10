#ifndef QAT_IR_TYPES_UNSIGNED_HPP
#define QAT_IR_TYPES_UNSIGNED_HPP

#include "./qat_type.hpp"

namespace qat {
namespace IR {

// Unsigned integer datatype in the language
class UnsignedType : public QatType {
private:
  const unsigned int bitWidth;

public:
  UnsignedType(const unsigned int _bitWidth);

  TypeKind typeKind() const;

  unsigned getBitwidth() const;

  bool isBitWidth(const unsigned int width) const;

  std::string toString() const;
};

} // namespace IR
} // namespace qat

#endif