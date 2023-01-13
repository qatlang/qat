#ifndef QAT_AST_TYPES_TUPLE_HPP
#define QAT_AST_TYPES_TUPLE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

#include <vector>

namespace qat::ast {

class TupleType : public QatType {
private:
  Vec<QatType*> types;
  bool          isPacked;

public:
  TupleType(Vec<QatType*> _types, bool _isPacked, bool _variable, FileRange _fileRange);

  Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif