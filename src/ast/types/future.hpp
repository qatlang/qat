#ifndef QAT_AST_TYPES_FUTURE_TYPE_HPP
#define QAT_AST_TYPES_FUTURE_TYPE_HPP

#include "qat_type.hpp"

namespace qat::ast {

class FutureType : public QatType {
private:
  ast::QatType* subType;
  bool          isPacked;

public:
  FutureType(bool _isPacked, ast::QatType* _subType, FileRange _fileRange)
      : QatType(_fileRange), subType(_subType), isPacked(_isPacked) {}

  useit static inline FutureType* create(bool isPacked, ast::QatType* subType, FileRange fileRange) {
    return std::construct_at(OwnNormal(FutureType), isPacked, subType, fileRange);
  }

  Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif