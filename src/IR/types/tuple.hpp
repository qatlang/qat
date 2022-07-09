#ifndef QAT_IR_TYPES_TUPLE_HPP
#define QAT_IR_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include <vector>

namespace qat {
namespace IR {

/**
 *  Tuples are product types. It is a defined fixed-length sequence of
 * other types
 *
 */
class TupleType : public QatType {
private:
  std::vector<QatType *> types;

  // Whether this tuple should be packed
  bool isPacked;

public:
  TupleType(const std::vector<QatType *> _types, const bool _isPacked);

  std::vector<QatType *> getSubTypes() const;

  QatType *getSubtypeAt(unsigned index);

  unsigned getSubTypeCount() const;

  bool isPackedTuple() const;

  TypeKind typeKind() const;
};
} // namespace IR
} // namespace qat

#endif