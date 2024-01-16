#ifndef QAT_AST_LINKED_GENERIC_HPP
#define QAT_AST_LINKED_GENERIC_HPP

#include "generic_abstract.hpp"
#include "qat_type.hpp"

namespace qat::ast {
class LinkedGeneric : public QatType {
  ast::GenericAbstractType* genAbs;

public:
  LinkedGeneric(ast::GenericAbstractType* _genAbs, FileRange _range) : QatType(_range), genAbs(_genAbs) {}

  useit static inline LinkedGeneric* create(ast::GenericAbstractType* _genAbs, FileRange _range) {
    return std::construct_at(OwnNormal(LinkedGeneric), _genAbs, _range);
  }

  useit IR::QatType* emit(IR::Context* ctx) final;
  useit AstTypeKind  typeKind() const final;
  useit Json         toJson() const final;
  useit String       toString() const final;
};

} // namespace qat::ast

#endif