#ifndef QAT_AST_TYPES_FUTURE_TYPE_HPP
#define QAT_AST_TYPES_FUTURE_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class FutureType : public QatType {
private:
  ast::QatType* subType;
  bool          isPacked;

public:
  FutureType(bool isPacked, ast::QatType* subType, FileRange fileRange);

  Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif