#ifndef QAT_AST_TYPES_TUPLE_HPP
#define QAT_AST_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

#include <vector>

namespace qat::ast {

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
  TupleType(const Vec<QatType *> _types, const bool _isPacked,
            const bool _variable, const utils::FileRange _fileRange);

  useit IR::QatType *emit(IR::Context *ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif