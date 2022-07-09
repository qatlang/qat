#ifndef QAT_IR_TYPES_QAT_TYPE_HPP
#define QAT_IR_TYPES_QAT_TYPE_HPP

#include "../../backend/cpp.hpp"
#include "../../backend/json.hpp"
#include "../../utils/file_placement.hpp"
#include "./type_kind.hpp"
#include "llvm/IR/Type.h"
#include <string>

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