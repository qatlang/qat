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
  std::vector<QatType *> types;

  // Whether this tuple should be packed
  bool isPacked;

public:
  TupleType(const std::vector<QatType *> _types, const bool _isPacked,
            const bool _variable, const utils::FileRange _fileRange);

  IR::QatType *emit(IR::Context *ctx);

  TypeKind typeKind();

  nuo::Json toJson() const;

  std::string toString() const;
};

} // namespace qat::ast

#endif