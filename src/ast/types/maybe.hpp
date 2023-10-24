#ifndef QAT_AST_MAYBE_HPP
#define QAT_AST_MAYBE_HPP

#include "qat_type.hpp"
namespace qat::ast {

class MaybeType : public QatType {
private:
  QatType* subTyp;
  bool     isPacked;

public:
  MaybeType(bool isPacled, QatType* subty, FileRange range);

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final { return TypeKind::maybe; }
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif