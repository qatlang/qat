#ifndef QAT_AST_LINKED_GENERIC_HPP
#define QAT_AST_LINKED_GENERIC_HPP

#include "generic_abstract.hpp"
#include "qat_type.hpp"

namespace qat::ast {
class LinkedGeneric : public QatType {
  ast::GenericAbstractType* genAbs;

public:
  LinkedGeneric(bool isVariable, ast::GenericAbstractType* genAbs, FileRange range);
  IR::QatType*   emit(IR::Context* ctx) final;
  useit TypeKind typeKind() const final;
  useit Json     toJson() const final;
  useit String   toString() const final;
};

} // namespace qat::ast

#endif