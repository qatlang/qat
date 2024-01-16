#ifndef QAT_AST_MAYBE_HPP
#define QAT_AST_MAYBE_HPP

#include "qat_type.hpp"
namespace qat::ast {

class MaybeType : public QatType {
private:
  QatType* subTyp;
  bool     isPacked;

public:
  MaybeType(bool _isPacked, QatType* _subType, FileRange _fileRange)
      : QatType(_fileRange), subTyp(_subType), isPacked(_isPacked) {}

  useit static inline MaybeType* create(bool _isPacked, QatType* _subType, FileRange _fileRange) {
    return std::construct_at(OwnNormal(MaybeType), _isPacked, _subType, _fileRange);
  }

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final { return AstTypeKind::MAYBE; }
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif