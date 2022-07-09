#ifndef QAT_IR_TYPES_QAT_TYPE_HPP
#define QAT_IR_TYPES_QAT_TYPE_HPP

#include "./type_kind.hpp"

namespace qat {
namespace IR {

// QatType is the base class for all types in the IR
class QatType {
public:
  QatType() {}

  virtual ~QatType(){};

  // TypeKind is used to detect variants of the QatType
  virtual TypeKind typeKind() const {};

  bool isSame(QatType *other) const;
};
} // namespace IR
} // namespace qat

#endif