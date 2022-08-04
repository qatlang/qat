#ifndef QAT_IR_TYPES_TUPLE_HPP
#define QAT_IR_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

#include <vector>

namespace qat::IR {

/**
 *  Tuples are product types. It is a defined fixed-length sequence of
 * other types
 *
 */
class TupleType : public QatType {
private:
  Vec<QatType *> types;

  // Whether this tuple should be packed
  bool isPacked;

public:
  TupleType(const Vec<QatType *> _types, const bool _isPacked);

  Vec<QatType *> getSubTypes() const;

  QatType *getSubtypeAt(u64 index);

  u64 getSubTypeCount() const;

  bool isPackedTuple() const;

  TypeKind typeKind() const;

  String toString() const;
};

} // namespace qat::IR

#endif