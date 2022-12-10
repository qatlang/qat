#ifndef QAT_AST_TYPES_STRING_SLICE_HPP
#define QAT_AST_TYPES_STRING_SLICE_HPP

#include "../../IR/context.hpp"
#include "./qat_type.hpp"

namespace qat::ast {

class StringSliceType : public QatType {
public:
  StringSliceType(bool _variable, FileRange _fileRange);

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit TypeKind     typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif