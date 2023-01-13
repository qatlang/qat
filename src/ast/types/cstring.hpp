#ifndef QAT_AST_TYPES_CSTRING_HPP
#define QAT_AST_TYPES_CSTRING_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class CStringType : public QatType {
public:
  CStringType(bool _variable, FileRange _fileRange);

  useit Maybe<usize> getTypeSizeInBits(IR::Context* ctx) const final;
  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif