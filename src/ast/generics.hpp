#ifndef QAT_AST_GENERICS_HPP
#define QAT_AST_GENERICS_HPP

#include "./types/qat_type.hpp"
#include "expression.hpp"
#include "node.hpp"

namespace qat::ast {

enum class FillGenericKind {
  typed,
  constant,
};

class FillGeneric {
  void*           data;
  FillGenericKind kind;

public:
  explicit FillGeneric(QatType* type);
  explicit FillGeneric(PrerunExpression* expression);

  useit bool isType() const;
  useit bool isConst() const;

  useit QatType*          asType() const;
  useit PrerunExpression* asConst() const;

  useit FileRange const& getRange() const;

  useit IR::GenericToFill* toFill(IR::Context* ctx) const;

  useit Json   toJson() const;
  useit String toString() const;
};

} // namespace qat::ast

#endif