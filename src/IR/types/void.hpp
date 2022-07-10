#ifndef QAT_IR_TYPES_VOID_HPP
#define QAT_IR_TYPES_VOID_HPP

#include "./qat_type.hpp"

namespace qat {
namespace IR {

class VoidType : public QatType {
public:
  VoidType() {}

  TypeKind typeKind() const;

  std::string toString() const;
};

} // namespace IR
} // namespace qat

#endif